/**
 * Sandwich Engine — Chat Interface Controller
 * Handles conversation state, streaming markdown rendering, code block copying, and input handling
 */

export class ChatController {
  constructor(api, elements) {
    this.api = api;
    this.elements = elements;
    this.messages = [];
    this.isGenerating = false;
    this.abortController = null;
    this.autoScroll = true;

    this.initEventListeners();
  }

  initEventListeners() {
    const { textarea, sendBtn, stopBtn, messagesContainer } = this.elements;

    // Auto-expanding textarea
    textarea.addEventListener('input', () => {
      textarea.style.height = 'auto';
      textarea.style.height = Math.min(textarea.scrollHeight, 180) + 'px';
    });

    // Enter to send, Shift+Enter for newline
    textarea.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
        this.handleSend();
      }
    });

    sendBtn.addEventListener('click', () => this.handleSend());
    stopBtn.addEventListener('click', () => this.handleStop());

    // Scroll listener for auto-scroll lock
    messagesContainer.addEventListener('scroll', () => {
      const { scrollTop, scrollHeight, clientHeight } = messagesContainer;
      this.autoScroll = (scrollHeight - scrollTop - clientHeight) < 50;
    });

    // Delegate copy buttons for code blocks
    messagesContainer.addEventListener('click', (e) => {
      const copyBtn = e.target.closest('.code-copy-btn');
      if (copyBtn) {
        const codeText = copyBtn.dataset.code;
        if (codeText) {
          navigator.clipboard.writeText(decodeURIComponent(codeText)).then(() => {
            const originalHTML = copyBtn.innerHTML;
            copyBtn.innerHTML = '✓ Copied!';
            setTimeout(() => { copyBtn.innerHTML = originalHTML; }, 1800);
          });
        }
      }
    });
  }

  async handleSend(customPrompt = null) {
    if (this.isGenerating) return;

    const promptText = (customPrompt || this.elements.textarea.value).trim();
    if (!promptText) return;

    // Reset textarea
    this.elements.textarea.value = '';
    this.elements.textarea.style.height = 'auto';

    // Hide welcome screen if first message
    if (this.elements.welcomeScreen) {
      this.elements.welcomeScreen.style.display = 'none';
    }

    // Add user message
    this.appendMessage('user', promptText);
    this.messages.push({ role: 'user', content: promptText });

    // Prepare assistant message bubble
    const assistantBubble = this.appendMessage('assistant', '');
    const contentEl = assistantBubble.querySelector('.bubble-text');
    const cursor = document.createElement('span');
    cursor.className = 'typing-cursor';
    contentEl.appendChild(cursor);

    this.isGenerating = true;
    this.elements.stopBtn.classList.add('active');
    this.elements.sendBtn.style.display = 'none';

    this.abortController = new AbortController();
    let accumulatedContent = '';
    const startTime = performance.now();

    await this.api.streamChatCompletion(
      this.messages,
      {
        signal: this.abortController.signal,
        temperature: 0.7,
        max_tokens: 4096
      },
      {
        onToken: (token) => {
          accumulatedContent += token;
          contentEl.innerHTML = this.renderMarkdown(accumulatedContent);
          contentEl.appendChild(cursor);

          if (this.autoScroll) {
            this.scrollToBottom();
          }

          // Trigger live throughput callback if registered
          if (this.onStreamToken) {
            this.onStreamToken();
          }
        },
        onDone: (timings, wasAborted) => {
          cursor.remove();
          this.isGenerating = false;
          this.elements.stopBtn.classList.remove('active');
          this.elements.sendBtn.style.display = 'inline-flex';

          if (accumulatedContent) {
            this.messages.push({ role: 'assistant', content: accumulatedContent });
          }

          // Add stats meta pill
          const totalMs = Math.round(performance.now() - startTime);
          let statsText = `${totalMs}ms`;
          if (timings && timings.predicted_per_second) {
            const tokPerSec = timings.predicted_per_second.toFixed(1);
            statsText = `${timings.predicted_n} tokens │ ${tokPerSec} tok/s │ ${totalMs}ms`;
          } else if (accumulatedContent) {
            const estTokens = Math.round(accumulatedContent.length / 4);
            const tokPerSec = (estTokens / (totalMs / 1000)).toFixed(1);
            statsText = `~${estTokens} tokens │ ${tokPerSec} tok/s │ ${totalMs}ms`;
          }

          const metaEl = document.createElement('div');
          metaEl.className = 'message-meta';
          metaEl.innerHTML = `<span class="badge">${statsText}</span>`;
          assistantBubble.appendChild(metaEl);

          if (this.autoScroll) {
            this.scrollToBottom();
          }
        },
        onError: (err) => {
          cursor.remove();
          this.isGenerating = false;
          this.elements.stopBtn.classList.remove('active');
          this.elements.sendBtn.style.display = 'inline-flex';
          contentEl.innerHTML = `<span style="color:var(--accent-rose);">Error: ${err.message}</span>`;
        }
      }
    );
  }

  handleStop() {
    if (this.abortController) {
      this.abortController.abort();
    }
  }

  appendMessage(role, text) {
    const row = document.createElement('div');
    row.className = `message-row ${role}`;

    const avatar = document.createElement('div');
    avatar.className = 'message-avatar';
    avatar.innerText = (role === 'user') ? 'U' : '🥪';

    const bubble = document.createElement('div');
    bubble.className = 'message-bubble';

    const textEl = document.createElement('div');
    textEl.className = 'bubble-text';
    textEl.innerHTML = this.renderMarkdown(text);
    bubble.appendChild(textEl);

    row.appendChild(avatar);
    row.appendChild(bubble);

    this.elements.messagesContainer.appendChild(row);
    this.scrollToBottom();
    return bubble;
  }

  scrollToBottom() {
    this.elements.messagesContainer.scrollTop = this.elements.messagesContainer.scrollHeight;
  }

  clearChat() {
    this.messages = [];
    this.elements.messagesContainer.innerHTML = '';
    if (this.elements.welcomeScreen) {
      this.elements.messagesContainer.appendChild(this.elements.welcomeScreen);
      this.elements.welcomeScreen.style.display = 'block';
    }
  }

  renderMarkdown(text) {
    if (!text) return '';

    // Escape raw HTML entities
    let escaped = text
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;');

    // Code blocks with syntax header and copy button
    escaped = escaped.replace(/```([a-zA-Z0-9_\-\+]*)\n([\s\S]*?)```/g, (match, lang, code) => {
      const language = lang.trim() || 'plaintext';
      const encodedCode = encodeURIComponent(code);
      return `<div class="code-container">
        <div class="code-header">
          <span>${language}</span>
          <button class="code-copy-btn" data-code="${encodedCode}">📋 Copy</button>
        </div>
        <pre><code>${code}</code></pre>
      </div>`;
    });

    // Inline code
    escaped = escaped.replace(/`([^`]+)`/g, '<code>$1</code>');

    // Bold & Italic
    escaped = escaped.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
    escaped = escaped.replace(/\*([^*]+)\*/g, '<em>$1</em>');

    // Headers
    escaped = escaped.replace(/^### (.*$)/gim, '<h3>$1</h3>');
    escaped = escaped.replace(/^## (.*$)/gim, '<h2>$1</h2>');
    escaped = escaped.replace(/^# (.*$)/gim, '<h1>$1</h1>');

    // Blockquotes
    escaped = escaped.replace(/^\> (.*$)/gim, '<blockquote>$1</blockquote>');

    // Paragraphs (double newlines)
    const paragraphs = escaped.split(/\n\n+/);
    return paragraphs.map(p => {
      if (p.startsWith('<div class="code-container">') || p.startsWith('<h') || p.startsWith('<blockquote')) {
        return p;
      }
      return `<p>${p.replace(/\n/g, '<br/>')}</p>`;
    }).join('');
  }
}
