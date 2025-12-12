package models

import "time"

// =========================================================
// Database Models (Map to ClickHouse)
// =========================================================

// LogEntry represents a single row in the 'sentry.logs' table.
// Field tags ensure correct mapping to JSON API and DB columns.
type LogEntry struct {
	ID           string    `json:"id" db:"id"`
	Timestamp    time.Time `json:"timestamp" db:"timestamp"`
	Host         string    `json:"host" db:"host"`
	Country      string    `json:"country" db:"country"`
	Service      string    `json:"service" db:"service"`
	Message      string    `json:"message" db:"message"`
	AnomalyScore float32   `json:"anomaly_score" db:"anomaly_score"`
	IsThreat     uint8     `json:"is_threat" db:"is_threat"`
}

// Stats represents the aggregation for the dashboard charts.
type Stats struct {
	TotalLogs    uint64 `json:"total_logs" db:"total_logs"`
	ThreatCount  uint64 `json:"threat_count" db:"threat_count"`
	EventsPerSec uint64 `json:"eps" db:"eps"`
}

// =========================================================
// API Request/Response Models
// =========================================================

// LoginRequest is the payload for POST /api/login
type LoginRequest struct {
	Username string `json:"username" binding:"required"`
	Password string `json:"password" binding:"required"`
}

// AuthResponse returns the JWT token
type AuthResponse struct {
	Token     string `json:"token"`
	ExpiresAt int64  `json:"expires_at"`
}

// SearchQuery defines filters for the Investigator page
type SearchQuery struct {
	StartTime   time.Time `json:"start_time"`
	EndTime     time.Time `json:"end_time"`
	MinScore    float32   `json:"min_score"` // e.g., 0.8 for threats only
	Service     string    `json:"service"`
	Limit       int       `json:"limit"`
	Offset      int       `json:"offset"`
}