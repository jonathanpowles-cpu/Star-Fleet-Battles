"""
TCP proxy: 127.0.0.1:16668  ->  server.sfbonline.com:6668
Logs all traffic to sfb_proxy.log
"""
import socket, threading, time, re

LISTEN_ADDR = ('127.0.0.1', 6668)       # loopback only
TARGET_ADDR = ('155.138.162.197', 6668)   # real current IP, bypasses hosts redirect
LOG_FILE    = 'sfb_proxy.log'

log_lock = threading.Lock()

# Redact credentials so the traffic log never stores a cleartext password.
_REDACT = [
    (re.compile(r'(LOGIN\s+\S+\s+)(\S+)', re.I),   r'\1***'),
    (re.compile(r'(/login\s+\S+\s+)(\S+)', re.I),  r'\1***'),
    (re.compile(r'(\bPASS\s+)(\S+)', re.I),        r'\1***'),
]

def redact(line):
    for pat, repl in _REDACT:
        line = pat.sub(repl, line)
    return line

def log(label, line):
    line = redact(line)
    ts  = time.strftime('%H:%M:%S')
    msg = f'[{ts}] {label} {line}'
    print(msg, flush=True)
    with log_lock:
        with open(LOG_FILE, 'a', encoding='utf-8') as f:
            f.write(msg + '\n')

def relay(src, dst, label):
    buf = b''
    try:
        while True:
            chunk = src.recv(4096)
            if not chunk:
                break
            dst.sendall(chunk)
            buf += chunk
            while b'\r\n' in buf:
                line, buf = buf.split(b'\r\n', 1)
                log(label, line.decode('utf-8', errors='replace'))
    except Exception as e:
        log(label, f'[relay ended: {e}]')

def handle(client, addr):
    log('---', f'Client connected from {addr}')
    try:
        server = socket.create_connection(TARGET_ADDR, timeout=10)
    except Exception as e:
        log('!!!', f'Cannot reach {TARGET_ADDR}: {e}')
        client.close()
        return
    # Kill Nagle buffering on both ends so relayed bytes go out instantly.
    client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    server.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    # create_connection leaves a 10s timeout on the upstream socket; clear it
    # so idle periods don't kill the relay and force client reconnects.
    server.settimeout(None)
    client.settimeout(None)

    # The real server takes ~6s (hostname/ident lookup) before it sends the
    # login prompt, and the client races all configured servers -- so the
    # proxied path loses to direct servers by a hair and gets aborted.
    # Inject the prompt instantly so the client commits to us and logs in.
    # The upstream buffers early commands, so forwarding them works fine.
    try:
        client.sendall(b':game.sfbonline.com NOTICE AUTH :*** Enter your login and password now.\r\n')
        log('P>C', '[injected login prompt to win race]')
    except Exception as e:
        log('!!!', f'inject failed: {e}')
    t1 = threading.Thread(target=relay, args=(client, server, 'C>S'), daemon=True)
    t2 = threading.Thread(target=relay, args=(server, client, 'S>C'), daemon=True)
    t1.start(); t2.start()
    t1.join(); t2.join()
    client.close(); server.close()
    log('---', 'Session ended')

srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(LISTEN_ADDR)
srv.listen(5)
log('===', f'Proxy on {LISTEN_ADDR[0]}:{LISTEN_ADDR[1]} -> {TARGET_ADDR[0]}:{TARGET_ADDR[1]}')

try:
    while True:
        conn, addr = srv.accept()
        threading.Thread(target=handle, args=(conn, addr), daemon=True).start()
except KeyboardInterrupt:
    pass
finally:
    srv.close()
