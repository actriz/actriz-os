# ===== Actriz OS — .zshrc =====
HISTFILE=~/.zsh_history
HISTSIZE=10000; SAVEHIST=10000
setopt HIST_IGNORE_DUPS HIST_IGNORE_SPACE SHARE_HISTORY INC_APPEND_HISTORY
autoload -Uz compinit; compinit -C
setopt AUTO_CD AUTO_PUSHD PUSHD_IGNORE_DUPS INTERACTIVE_COMMENTS
export EDITOR=${EDITOR:-nano} VISUAL=$EDITOR

# --- Highlighting: solo abc en rosado, resto neutro ---
source /usr/share/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh

# Apagar TODOS los colores por defecto
ZSH_HIGHLIGHT_STYLES=()
ZSH_HIGHLIGHT_PATTERNS=()

# Solo "abc" y "ABC" se pintan
ZSH_HIGHLIGHT_PATTERNS+=('*abc*' 'fg=205,bold')
ZSH_HIGHLIGHT_PATTERNS+=('*ABC*' 'fg=205,bold')

# --- Prompt: usuario/host azul claro, @ amarillo. SIN hora. ---
PROMPT='%F{75}%n%F{228}@%F{75}%m%f %F{111}%~%f %# '
RPROMPT=''

alias ls='ls --color=auto'
alias ll='ls -lah'
