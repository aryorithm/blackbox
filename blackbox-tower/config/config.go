package config

import (
	"log"
	"os"
	"strconv"
)

// Config holds all runtime settings for the Go API
type Config struct {
	ServerPort    string
	ClickHouseURL string
	RedisHost     string
	RedisPort     string
	JWTSecret     string
}

// LoadConfig reads env vars or sets defaults
func LoadConfig() *Config {
	return &Config{
		ServerPort:    getEnv("TOWER_PORT", "8080"),
		ClickHouseURL: getEnv("BLACKBOX_CLICKHOUSE_URL", "tcp://localhost:9000"),
		RedisHost:     getEnv("BLACKBOX_REDIS_HOST", "localhost"),
		RedisPort:     getEnv("BLACKBOX_REDIS_PORT", "6379"),
		JWTSecret:     getEnv("JWT_SECRET", "change_this_secret_in_production"),
	}
}

// Helper to read ENV with fallback
func getEnv(key, fallback string) string {
	if value, exists := os.LookupEnv(key); exists {
		return value
	}
	return fallback
}

// Helper to get Int ENV (useful if we expand config later)
func getEnvInt(key string, fallback int) int {
	strValue := getEnv(key, "")
	if strValue == "" {
		return fallback
	}
	val, err := strconv.Atoi(strValue)
	if err != nil {
		log.Printf("Invalid int for env %s: %v. Using default.", key, err)
		return fallback
	}
	return val
}