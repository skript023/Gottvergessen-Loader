<script setup lang="ts">
import { computed, nextTick, ref, watch } from 'vue';
import { useRouter } from 'vue-router';
import { useGamesStore } from '../stores/games';

const props = defineProps<{ open: boolean }>();
const emit = defineEmits<{ (e: 'update:open', value: boolean): void }>();

const router = useRouter();
const gamesStore = useGamesStore();

const inputEl = ref<HTMLInputElement | null>(null);
const activeIndex = ref(0);
const failedIcons = ref<Record<string, boolean>>({});

const hasQuery = computed(() => gamesStore.searchQuery.trim().length > 0);
const results = computed(() => (hasQuery.value ? gamesStore.filteredGames : gamesStore.recentGames));

watch(
  () => props.open,
  async (open) => {
    if (!open) return;
    gamesStore.searchQuery = '';
    activeIndex.value = 0;
    await nextTick();
    inputEl.value?.focus();
  }
);

// A new result set invalidates whatever row was highlighted before.
watch(results, () => {
  activeIndex.value = 0;
});

function close() {
  emit('update:open', false);
  gamesStore.searchQuery = '';
}

function openGame(gameId: string) {
  close();
  router.push({ path: '/dashboard', query: { game: gameId } });
}

// A term with no game behind it is often a PID or an executable name, so hand
// it over to the process manager instead of ending on a dead end.
function openProcessManager() {
  const term = gamesStore.searchQuery.trim();
  close();
  router.push({ path: '/processes', query: term ? { q: term } : {} });
}

function moveActive(step: number) {
  const total = results.value.length;
  if (total === 0) return;
  activeIndex.value = (activeIndex.value + step + total) % total;
}

function handleEnter() {
  const game = results.value[activeIndex.value];
  if (game) openGame(game.id);
}
</script>

<template>
  <Teleport to="body">
    <transition name="menu-pop">
      <div v-if="open" class="search-palette-layer">
        <div class="search-palette-scrim" @click="close"></div>

        <div class="search-palette" role="dialog" aria-modal="true" aria-label="Search games">
          <div class="search-palette-field">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <circle cx="11" cy="11" r="8" /><line x1="21" y1="21" x2="16.65" y2="16.65" />
            </svg>
            <input
              ref="inputEl"
              v-model="gamesStore.searchQuery"
              type="text"
              class="search-palette-input"
              placeholder="Search"
              spellcheck="false"
              autocomplete="off"
              @keydown.down.prevent="moveActive(1)"
              @keydown.up.prevent="moveActive(-1)"
              @keydown.enter.prevent="handleEnter"
              @keydown.esc.prevent="close"
            />
          </div>

          <div class="search-palette-body">
            <div v-if="results.length === 0" class="search-palette-empty">
              <p>There are no games that match your search. Please search again.</p>
              <button class="search-palette-fallback" @click="openProcessManager">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                  <rect x="4" y="4" width="16" height="16" rx="2"/><rect x="9" y="9" width="6" height="6"/>
                  <line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/>
                  <line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/>
                  <line x1="20" y1="9" x2="23" y2="9"/><line x1="20" y1="15" x2="23" y2="15"/>
                  <line x1="1" y1="9" x2="4" y2="9"/><line x1="1" y1="15" x2="4" y2="15"/>
                </svg>
                <span>Search &ldquo;{{ gamesStore.searchQuery.trim() }}&rdquo; in Process Manager</span>
              </button>
            </div>

            <template v-else>
              <div v-if="!hasQuery" class="search-palette-label">Recent games</div>

              <button
                v-for="(game, index) in results"
                :key="game.id"
                class="search-palette-item"
                :class="{ 'is-active': index === activeIndex }"
                @mousemove="activeIndex = index"
                @click="openGame(game.id)"
              >
                <span class="search-palette-thumb">
                  <img
                    v-if="game.iconUrl && !failedIcons[game.id]"
                    :src="game.iconUrl"
                    :alt="game.name"
                    loading="lazy"
                    @error="failedIcons[game.id] = true"
                  />
                  <span v-else>{{ game.name.substring(0, 1).toUpperCase() }}</span>
                </span>
                <span class="search-palette-name">{{ game.name }}</span>
              </button>
            </template>
          </div>
        </div>
      </div>
    </transition>
  </Teleport>
</template>
