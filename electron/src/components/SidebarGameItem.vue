<script setup lang="ts">
import { ref, computed } from 'vue';
import type { InstalledGameItem } from '../types/loader';
import { useGamesStore } from '../stores/games';

const props = defineProps<{ game: InstalledGameItem }>();
const emit = defineEmits<{ (e: 'select', gameId: string): void }>();

const gamesStore = useGamesStore();
const failedIconUrl = ref('');

const isSelected = computed(() => gamesStore.selectedGameId === props.game.id);
const isPlaying = computed(() => gamesStore.launchStatus === 'running' && gamesStore.activeGameId === props.game.id);
</script>

<template>
  <div
    class="game-list-item"
    :class="{ active: isSelected, running: isPlaying }"
    @click="emit('select', game.id)"
  >
    <div class="game-item-thumb">
      <img
        v-if="game.iconUrl && failedIconUrl !== game.iconUrl"
        :src="game.iconUrl"
        :alt="game.name"
        loading="lazy"
        @error="failedIconUrl = game.iconUrl || ''"
      />
      <div v-else class="game-thumb-fallback">
        {{ game.name.substring(0, 1).toUpperCase() }}
      </div>
    </div>

    <div class="game-item-text-wrap">
      <span class="game-item-name" :title="game.name">
        {{ game.name }}
      </span>
      <span
        v-if="gamesStore.getGameMatchedBinary(game)"
        class="badge-game-mod"
        title="Assigned Cloud Mod Payload"
      >
        MOD
      </span>
    </div>

    <span v-if="isPlaying" class="game-playing-pulse" title="Playing Now"></span>
  </div>
</template>
