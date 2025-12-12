import React, { useEffect, useRef } from 'react';
import { FixedSizeList as List } from 'react-window';
import AutoSizer from 'react-virtualized-auto-sizer';
import { useTelemetryStore } from '../../store/useTelemetryStore';
import { LogEntry } from '../../types/models';

// Row Component (rendered for each log)
const LogRow = ({ index, style, data }: { index: number; style: React.CSSProperties; data: LogEntry[] }) => {
    const log = data[index];

    // Dynamic styling based on threat level
    let colorClass = "text-green-500"; // Default Matrix Green
    if (log.is_threat) colorClass = "text-red-500 font-bold animate-pulse";
    else if (log.anomaly_score > 0.5) colorClass = "text-yellow-500";

    return (
        <div style={style} className={`flex items-center space-x-4 font-mono text-xs hover:bg-white/5 px-2 ${colorClass}`}>
            <span className="w-20 opacity-50">{log.timestamp.split('T')[1].replace('Z','')}</span>
            <span className="w-24 text-blue-400">{log.host}</span>
            <span className="w-20 text-purple-400">{log.service}</span>
            <span className="w-16">{log.country}</span>
            <span className="flex-1 truncate">{log.message}</span>
            <span className="w-12 text-right opacity-70">{(log.anomaly_score * 100).toFixed(0)}%</span>
        </div>
    );
};

export const LogViewer = () => {
    const logs = useTelemetryStore((state) => state.logs);
    const listRef = useRef<List>(null);

    // Auto-scroll to bottom
    useEffect(() => {
        if (listRef.current && logs.length > 0) {
            listRef.current.scrollToItem(logs.length);
        }
    }, [logs.length]);

    return (
        <div className="flex-1 bg-black/90 border border-white/10 rounded-lg overflow-hidden relative">
            {/* Header */}
            <div className="flex items-center px-4 py-2 bg-white/5 text-xs font-bold text-gray-400 border-b border-white/10">
                <span className="w-20">TIME</span>
                <span className="w-24">HOST</span>
                <span className="w-20">SERVICE</span>
                <span className="w-16">GEO</span>
                <span className="flex-1">MESSAGE</span>
                <span className="w-12 text-right">SCORE</span>
            </div>

            {/* Virtualized List */}
            <div className="h-full">
                <AutoSizer>
                    {({ height, width }) => (
                        <List
                            ref={listRef}
                            height={height - 32} // Subtract header height
                            itemCount={logs.length}
                            itemSize={24} // Height of each row in px
                            width={width}
                            itemData={logs}
                        >
                            {LogRow}
                        </List>
                    )}
                </AutoSizer>
            </div>
        </div>
    );
};