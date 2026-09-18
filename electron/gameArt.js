const fs = require("node:fs");
const path = require("node:path");
const { app } = require("electron");

/**
 * Resolves game artwork from Steam store metadata (the same asset set SteamDB shows):
 *  - iconUrl:   the Steam community app icon
 *  - bannerUrl: the library hero artwork (falls back to the page background / header)
 * Steam games are looked up by appId, every other game by matching its title
 * against the Steam store search.
 */

const STORE_ASSET_BASE = "https://shared.akamai.steamstatic.com/store_item_assets/";
const COMMUNITY_ICON_BASE = "https://cdn.cloudflare.steamstatic.com/steamcommunity/public/images/apps/";
const HIT_TTL_MS = 7 * 24 * 60 * 60 * 1000;
const MISS_TTL_MS = 12 * 60 * 60 * 1000;
const REQUEST_TIMEOUT_MS = 8000;
const CONCURRENCY = 4;

let cache = null;

function getCachePath() {
  return path.join(app.getPath("userData"), "game_art_cache.json");
}

function loadCache() {
  if (cache) return cache;
  try {
    cache = JSON.parse(fs.readFileSync(getCachePath(), "utf-8")) || {};
  } catch (_) {
    cache = {};
  }
  return cache;
}

function saveCache() {
  try {
    fs.writeFileSync(getCachePath(), JSON.stringify(cache, null, 2), "utf-8");
  } catch (err) {
    console.error("[GameArt] Failed saving cache:", err);
  }
}

function cacheKey(game) {
  return game.platform === "steam" && game.appId
    ? `steam:${game.appId}`
    : `title:${normalizeTitle(game.name)}`;
}

function isFresh(entry) {
  if (!entry || !entry.checkedAt) return false;
  const ttl = entry.iconUrl || entry.bannerUrl ? HIT_TTL_MS : MISS_TTL_MS;
  return Date.now() - entry.checkedAt < ttl;
}

function normalizeTitle(name) {
  return String(name || "")
    .toLowerCase()
    .replace(/[™®©]/g, "")
    .replace(/[^a-z0-9]+/g, " ")
    .trim();
}

async function fetchJson(url) {
  const res = await fetch(url, { signal: AbortSignal.timeout(REQUEST_TIMEOUT_MS) });
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

async function findAppIdByTitle(title) {
  const wanted = normalizeTitle(title);
  if (!wanted) return null;
  const url = `https://store.steampowered.com/api/storesearch/?term=${encodeURIComponent(title)}&l=english&cc=US`;
  const data = await fetchJson(url);
  const items = (data && data.items) || [];
  const exact = items.find((item) => normalizeTitle(item.name) === wanted);
  return exact ? String(exact.id) : null;
}

async function fetchStoreAssets(appId) {
  const input = {
    ids: [{ appid: Number(appId) }],
    context: { language: "english", country_code: "US" },
    data_request: { include_assets: true }
  };
  const url = `https://api.steampowered.com/IStoreBrowseService/GetItems/v1/?input_json=${encodeURIComponent(JSON.stringify(input))}`;
  const data = await fetchJson(url);
  const item = data && data.response && data.response.store_items && data.response.store_items[0];
  return item && item.success === 1 ? item.assets || null : null;
}

function buildArt(appId, assets) {
  if (!assets) return { iconUrl: "", bannerUrl: "" };
  const format = assets.asset_url_format || `steam/apps/${appId}/\${FILENAME}`;
  const assetUrl = (file) => (file ? STORE_ASSET_BASE + format.replace("${FILENAME}", file) : "");

  return {
    iconUrl: assets.community_icon ? `${COMMUNITY_ICON_BASE}${appId}/${assets.community_icon}.jpg` : "",
    bannerUrl:
      assetUrl(assets.library_hero) ||
      assetUrl(assets.raw_page_background) ||
      assetUrl(assets.header)
  };
}

async function resolveOne(game) {
  const appId = game.platform === "steam" && game.appId ? game.appId : await findAppIdByTitle(game.name);
  if (!appId) return { appId: null, iconUrl: "", bannerUrl: "" };
  const assets = await fetchStoreAssets(appId);
  return { appId, ...buildArt(appId, assets) };
}

function mergeArt(game, entry) {
  if (!entry) return game;
  return {
    ...game,
    iconUrl: entry.iconUrl || game.iconUrl || "",
    bannerUrl: entry.bannerUrl || game.bannerUrl || ""
  };
}

/** Apply already cached artwork without touching the network. */
function applyCachedArt(games) {
  const store = loadCache();
  return games.map((game) => mergeArt(game, store[cacheKey(game)]));
}

/** Resolve missing or stale artwork, then return the games with artwork applied. */
async function resolveArt(games) {
  const store = loadCache();
  const pending = [];
  const seen = new Set();
  for (const game of games) {
    const key = cacheKey(game);
    if (seen.has(key) || isFresh(store[key])) continue;
    seen.add(key);
    pending.push({ key, game });
  }

  let changed = false;
  let next = 0;
  async function worker() {
    while (next < pending.length) {
      const { key, game } = pending[next++];
      try {
        store[key] = { ...(await resolveOne(game)), checkedAt: Date.now() };
        changed = true;
      } catch (err) {
        // Network hiccup: keep any previous entry and retry on the next scan.
        console.warn(`[GameArt] Could not resolve art for "${game.name}": ${err.message}`);
      }
    }
  }
  await Promise.all(Array.from({ length: Math.min(CONCURRENCY, pending.length) }, worker));

  if (changed) saveCache();
  return games.map((game) => mergeArt(game, store[cacheKey(game)]));
}

module.exports = { applyCachedArt, resolveArt };
