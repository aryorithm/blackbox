import client from '../api/client';

// Example: Fetching alerts
const fetchAlerts = async () => {
    try {
        // This automatically goes to http://<SERVER-IP>:8080/api/alerts
        // AND automatically attaches the Bearer Token.
        const response = await client.get('/api/alerts');
        return response.data;
    } catch (error) {
        console.error("Failed to connect to Tower:", error);
    }
};