export interface ProcessItem {
  name: string;
  pid: number;
  arch: string;
  accessible: boolean;
}

export interface BinaryItem {
  id: string;
  name: string;
  game?: string;
  file_name?: string;
  filename?: string;
  version?: string;
  arch?: string;
  description?: string;
  target_process?: string;
  injection_mode?: number;
  expiry_date?: string;
  expired_date?: string;
  license_status?: string;
  license_key?: string;
}

export interface UserLicenseItem {
  id?: string;
  license_id?: string;
  license_key?: string;
  product_id?: string;
  product_name?: string;
  product?: {
    id?: string;
    name?: string;
  };
  issued_at?: string;
  created_at?: string;
  expiry_date?: string;
  expired_date?: string;
  status?: string;
}

export interface UserRoleItem {
  id?: string;
  name?: string;
  role_name?: string;
  description?: string;
  user_type?: string;
}

export interface UserProfile {
  username: string;
  firstname?: string;
  lastname?: string;
  fullname?: string;
  role?: string | UserRoleItem;
  roles?: (string | UserRoleItem)[];
  user_type?: string;
  expired_date?: string;
  expiry_date?: string;
  expires_at?: string;
  licenses?: UserLicenseItem[];
}

export interface LoginCredentials {
  username: string;
  password: string;
  rememberMe?: boolean;
}

export interface SessionInfo {
  token: string;
  hwid: string;
  backendUrl: string;
  deviceName?: string;
}

export interface LoginResult {
  binaries: BinaryItem[];
  profile: UserProfile;
  token?: string;
  hwid?: string;
  backendUrl?: string;
  deviceName?: string;
}

export interface OperationStatus {
  progress: number;
  stage: string;
}

export interface InjectRequest {
  pid: number;
  processName: string;
  binaryIndex: number;
  mode: number;
}

export interface InstalledGameItem {
  id: string;
  appId?: string;
  name: string;
  platform: 'steam' | 'epic' | 'custom' | string;
  installDir: string;
  exeName: string;
  launchUri: string;
  iconUrl?: string;
  bannerUrl?: string;
  isCustom?: boolean;
}

export interface BrowseExecutableResult {
  exePath: string;
  exeName: string;
  name: string;
  installDir: string;
}

export interface PlayAndInjectParams {
  launchUri?: string;
  exePath?: string;
  targetProcess?: string;
  binaryIndex?: number;
  mode?: number;
}

export interface PlayAndInjectResult {
  success: boolean;
  pid?: number;
  processName?: string;
  injected?: boolean;
  message?: string;
}

export interface WindowControlsApi {
  minimize(): Promise<boolean>;
  maximize(): Promise<boolean>;
  close(): Promise<boolean>;
  isMaximized(): Promise<boolean>;
}

export interface LoaderApi {
  listProcesses(): Promise<ProcessItem[]>;
  operationStatus(): Promise<OperationStatus>;
  login(credentials: LoginCredentials): Promise<LoginResult>;
  restoreSession(): Promise<LoginResult | null>;
  logout(): Promise<boolean>;
  refreshBinaries(): Promise<BinaryItem[]>;
  saveBinarySettings(binaryId: string, targetProcess: string, mode: number): Promise<boolean>;
  inject(request: InjectRequest): Promise<boolean>;
  getSessionInfo(): Promise<SessionInfo>;
  listInstalledGames(): Promise<InstalledGameItem[]>;
  browseGameExecutable(): Promise<BrowseExecutableResult | null>;
  addCustomGame(data: { name: string; exePath: string }): Promise<InstalledGameItem[]>;
  removeCustomGame(gameId: string): Promise<InstalledGameItem[]>;
  playAndInject(params: PlayAndInjectParams): Promise<PlayAndInjectResult>;
  killGameProcess(pid: number): Promise<boolean>;
  window?: WindowControlsApi;
}

declare global {
  interface Window {
    loader?: LoaderApi;
    ellohim?: {
      window?: WindowControlsApi;
    };
  }
}

