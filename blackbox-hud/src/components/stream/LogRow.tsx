import { LogEvent } from '../../types/models'; // Include the header

interface Props {
    data: LogEvent;
    style: React.CSSProperties; // Passed by the virtualizer
}

export const LogRow: React.FC<Props> = ({ data, style }) => {
    // Logic: Determine color based on score
    const colorClass = data.anomaly_score > 0.8 ? 'text-red-500' : 'text-green-500';

    return (
        <div style={style} className={`font-mono text-xs ${colorClass}`}>
            <span>{data.timestamp}</span>
            <span className="mx-2">[{data.service}]</span>
            <span>{data.message}</span>
        </div>
    );
};
