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
}

export interface UserProfile {
  username: string;
  firstname?: string;
  lastname?: string;
  fullname?: string;
  role?: string;
  expired_date?: string;
  expiry_date?: string;
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
}

declare global {
  interface Window {
    loader?: LoaderApi;
  }
}
