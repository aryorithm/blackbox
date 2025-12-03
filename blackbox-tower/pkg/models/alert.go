package models

import "time"

// Alert represents a high-priority threat detected by xInfer
type Alert struct {
    Timestamp   time.Time `json:"timestamp" db:"timestamp"`
    Severity    string    `json:"severity" db:"severity"`
    RuleName    string    `json:"rule_name" db:"rule_name"`
    SourceIP    string    `json:"source_ip" db:"source_ip"`
    AnomalyScore float64  `json:"anomaly_score" db:"anomaly_score"`
    RawLog      string    `json:"raw_log" db:"raw_log"`
}
