package api_test

import (
	"net/http"
	"testing"
	"time"
)

// This assumes blackbox-tower is running via Docker
func TestHealthEndpoint(t *testing.T) {
	client := &http.Client{Timeout: 2 * time.Second}

	resp, err := client.Get("http://localhost:8080/api/v1/health")
	if err != nil {
		t.Fatalf("Failed to call API: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		t.Errorf("Expected status 200, got %d", resp.StatusCode)
	}
}