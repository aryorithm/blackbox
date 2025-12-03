import { LogEvent } from './models';

export interface TelemetryState {
    logs: LogEvent[];
    isConnected: boolean;
    eps: number; // Events Per Second

    // Actions
    addLog: (log: LogEvent) => void;
    setConnection: (status: boolean) => void;
    clearBuffer: () => void;
}
