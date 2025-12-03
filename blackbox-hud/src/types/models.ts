// Define what a "Log" looks like exactly
export interface LogEvent {
    id: string;
    timestamp: string;
    source_ip: string;
    service: string;
    message: string;
    anomaly_score: number; // 0.0 to 1.0
    is_threat: boolean;
}

// Define what the Red/Green/Blue status means
export type ThreatLevel = 'LOW' | 'MEDIUM' | 'HIGH' | 'CRITICAL';