import { createRouter, createWebHashHistory } from 'vue-router';
import Login from '../modules/auth/Login.vue';
import Dashboard from '../modules/dashboard/Dashboard.vue';
import CatalogManager from '../modules/catalog/CatalogManager.vue';
import ProcessManager from '../modules/process/ProcessManager.vue';
import Diagnostics from '../modules/diagnostics/Diagnostics.vue';
import { useAuthStore } from '../stores/auth';

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: Login,
    meta: { title: 'Sign In', guestOnly: true }
  },
  {
    path: '/',
    redirect: '/dashboard'
  },
  {
    path: '/dashboard',
    name: 'Dashboard',
    component: Dashboard,
    meta: { title: 'Dashboard Overview', requiresAuth: true }
  },
  {
    path: '/catalog',
    name: 'Catalog',
    component: CatalogManager,
    meta: { title: 'Binaries Catalog', requiresAuth: true }
  },
  {
    path: '/processes',
    name: 'Processes',
    component: ProcessManager,
    meta: { title: 'Process Manager', requiresAuth: true }
  },
  {
    path: '/diagnostics',
    name: 'Diagnostics',
    component: Diagnostics,
    meta: { title: 'Live Diagnostics', requiresAuth: true }
  },
  {
    path: '/:pathMatch(.*)*',
    redirect: '/dashboard'
  }
];

const router = createRouter({
  history: createWebHashHistory(),
  routes
});

router.beforeEach(async (to, _from, next) => {
  const auth = useAuthStore();

  // If initial load and session check hasn't finished, restore session
  if (auth.isRestoring) {
    await auth.restoreSession();
  }

  if (to.meta.requiresAuth && !auth.isAuthenticated) {
    next({ name: 'Login' });
  } else if (to.meta.guestOnly && auth.isAuthenticated) {
    next({ name: 'Dashboard' });
  } else {
    next();
  }
});

export default router;
