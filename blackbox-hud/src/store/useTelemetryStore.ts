import { create } from 'zustand';
import { LogEntry, DashboardStats } from '../types/models';

interface TelemetryState {
    logs: LogEntry[];
    stats: DashboardStats;
    isConnected: boolean;
    isPaused: boolean;

    // Actions
    addLog: (log: LogEntry) => void;
    setStats: (stats: DashboardStats) => void;
    setConnection: (status: boolean) => void;
    togglePause: () => void;
    clearLogs: () => void;
}

const MAX_LOG_BUFFER = 2000; // Keep only last 2000 logs in RAM

export const useTelemetryStore = create<TelemetryState>((set) => ({
    logs: [],
    stats: { total_logs: 0, threat_count: 0, eps: 0 },
    isConnected: false,
    isPaused: false,

    addLog: (newLog) => set((state) => {
        if (state.isPaused) return state; // Don't update if paused

        // Efficient array update
        // We append the new log and slice to keep size constant
        const newLogs = [...state.logs, newLog];
        if (newLogs.length > MAX_LOG_BUFFER) {
            newLogs.shift(); // Remove oldest
        }

        // Optimistic stats update (Real stats come from API polling)
        const newStats = { ...state.stats };
        newStats.total_logs++;
        if (newLog.is_threat) newStats.threat_count++;

        return { logs: newLogs, stats: newStats };
    }),

    setStats: (newStats) => set({ stats: newStats }),

    setConnection: (status) => set({ isConnected: status }),

    togglePause: () => set((state) => ({ isPaused: !state.isPaused })),

    clearLogs: () => set({ logs: [] })
}));