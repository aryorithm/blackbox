// Run starts the infinite loop that listens for Redis messages
func (h *Hub) Run() {
    for {
        select {
        case client := <-h.register:
            h.clients[client] = true
        case message := <-h.broadcast:
            // Send the message to all connected browsers
            for client := range h.clients {
                client.send <- message
            }
        }
    }
}
