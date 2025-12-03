// GetRecentAlerts fetches the last N alerts from the Vault
func (r *Repository) GetRecentAlerts(limit int) ([]models.Alert, error) {
    var alerts []models.Alert
    // This SQL must match your ClickHouse table schema
    query := `SELECT timestamp, severity, rule_name, source_ip, anomaly_score, raw_log
              FROM sentry.alerts
              ORDER BY timestamp DESC LIMIT ?`

    err := r.conn.Select(&alerts, query, limit)
    return alerts, err
}
