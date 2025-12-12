import { useEffect, useRef } from 'react';
import { useTelemetryStore } from '../store/useTelemetryStore';
import { LogEntry } from '../types/models';

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:8080/ws';
const RECONNECT_INTERVAL = 3000;

export const useWebSocket = () => {
    const ws = useRef<WebSocket | null>(null);
    const { addLog, setConnection } = useTelemetryStore();

    // Use a ref to prevent reconnection loops in React.StrictMode
    const isConnecting = useRef(false);

    const connect = () => {
        if (isConnecting.current || ws.current?.readyState === WebSocket.OPEN) return;

        isConnecting.current = true;
        console.log(`[WS] Connecting to ${WS_URL}...`);

        const socket = new WebSocket(WS_URL);

        socket.onopen = () => {
            console.log('[WS] Connected');
            setConnection(true);
            isConnecting.current = false;
        };

        socket.onclose = () => {
            console.log('[WS] Disconnected');
            setConnection(false);
            ws.current = null;
            isConnecting.current = false;

            // Auto-reconnect
            setTimeout(connect, RECONNECT_INTERVAL);
        };

        socket.onerror = (error) => {
            console.error('[WS] Error:', error);
            socket.close();
        };

        socket.onmessage = (event) => {
            try {
                // Parse the JSON string from Redis/Go
                const data: LogEntry = JSON.parse(event.data);

                // Push to Zustand store
                addLog(data);
            } catch (err) {
                console.error('[WS] Parse error:', err);
            }
        };

        ws.current = socket;
    };

    useEffect(() => {
        connect();

        // Cleanup on unmount
        return () => {
            if (ws.current) {
                ws.current.close();
            }
        };
    }, []);
};