/**
 * Sandwich Engine — Web GUI API Communication Layer
 * Connects with server REST & SSE endpoints: /health, /v1/models, /slots, /props, /v1/chat/completions
 */

export class SandwichAPI {
  constructor(baseUrl = '') {
    this.baseUrl = baseUrl;
  }

  async checkHealth() {
    try {
      const res = await fetch(`${this.baseUrl}/health`);
      if (!res.ok) return { status: 'error', code: res.status };
      return await res.json();
    } catch (err) {
      return { status: 'offline', error: err.message };
    }
  }

  async getModels() {
    try {
      const res = await fetch(`${this.baseUrl}/v1/models`);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      return data.data || [];
    } catch (err) {
      console.warn('Failed to fetch /v1/models:', err);
      return [];
    }
  }

  async getProps() {
    try {
      const res = await fetch(`${this.baseUrl}/props`);
      if (!res.ok) return null;
      return await res.json();
    } catch (err) {
      return null;
    }
  }

  async getSlots() {
    try {
      const res = await fetch(`${this.baseUrl}/slots`);
      if (!res.ok) return [];
      return await res.json();
    } catch (err) {
      return [];
    }
  }

  async streamChatCompletion(messages, options = {}, callbacks = {}) {
    const { onToken, onDone, onError } = callbacks;
    const signal = options.signal;

    const payload = {
      messages: messages,
      stream: true,
      temperature: options.temperature ?? 0.7,
      top_p: options.top_p ?? 0.9,
      max_tokens: options.max_tokens ?? 2048
    };

    try {
      const response = await fetch(`${this.baseUrl}/v1/chat/completions`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'text/event-stream'
        },
        body: JSON.stringify(payload),
        signal: signal
      });

      if (!response.ok) {
        const errorText = await response.text();
        throw new Error(`Server returned ${response.status}: ${errorText}`);
      }

      const reader = response.body.getReader();
      const decoder = new TextDecoder('utf-8');
      let buffer = '';
      let timings = null;

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split('\n');
        buffer = lines.pop(); // Keep partial line in buffer

        for (const line of lines) {
          const trimmed = line.trim();
          if (!trimmed || trimmed.startsWith(':')) continue;

          if (trimmed.startsWith('data: ')) {
            const dataStr = trimmed.slice(6);
            if (dataStr === '[DONE]') {
              continue;
            }

            try {
              const chunk = JSON.parse(dataStr);
              if (chunk.timings) {
                timings = chunk.timings;
              }
              const delta = chunk.choices?.[0]?.delta?.content;
              if (delta && onToken) {
                onToken(delta);
              }
            } catch (jsonErr) {
              // Ignore non-JSON heartbeat
            }
          }
        }
      }

      if (onDone) {
        onDone(timings);
      }
    } catch (err) {
      if (err.name === 'AbortError') {
        if (onDone) onDone(null, true);
      } else {
        if (onError) onError(err);
      }
    }
  }
}
