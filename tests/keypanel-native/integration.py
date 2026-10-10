import pathlib, socket, subprocess, struct, tempfile, threading, time, sys
exe=pathlib.Path(sys.argv[1]).resolve()
# QNX /tmp is unsuitable on this vehicle. Current-session pointer lives on the SD, with no rename.
source=pathlib.Path('src/native/keypanel/mibr_keypanel_native.c').read_text()
assert 'rename(' not in source, 'QNX pointer must not use rename syscall'
assert '#define POINTER "./mibr-keypanel-native-current"' in source
sync=b'\x80MLP'
def frm(msg):
    msg=msg.encode()
    payload=b'\x01\x01'+b'\x00'*10+struct.pack('>H',len(msg))+msg
    return struct.pack('>I',len(payload)+22)+sync+b'\0'*12+b'\0\xff'+struct.pack('>I',len(payload))+payload
# Server sends two lines with time for annotation in between.
evt=threading.Event()
def server():
    with socket.socket() as s:
        s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        s.bind(('127.0.0.1',15001));s.listen(1);evt.set()
        c,_=s.accept()
        with c:
            a=frm('[AslTargetSystemKeyPanelHandling] HK Received: KBD[4] KEY[50] KST[1]')
            for chunk in (a[:3],a[3:9],a[9:22],a[22:]):
                c.sendall(chunk);time.sleep(.06)
            time.sleep(.9)
            b=frm('[AslTargetSystemKeyPanelHandling] HK Received: KBD[4] KEY[50] KST[3]')
            c.sendall(b)
            c.sendall(frm('[AslTargetSystemKeyPanelHandling] HK Received: KBD[4] KEY[50] KST[0]'))
            time.sleep(.3)
th=threading.Thread(target=server);th.start();assert evt.wait(2)
with tempfile.TemporaryDirectory() as d:
    proc=subprocess.Popen([str(exe),'--capture','--out','.','--seconds','10'],cwd=d,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    for _ in range(40):
        if (pathlib.Path(d)/'mibr-keypanel-native-current').exists():break
        time.sleep(.03)
    time.sleep(.45)
    note=subprocess.run([str(exe),'--note','6 kurz'],cwd=d,capture_output=True,text=True)
    print('NOTE:',note.returncode,note.stdout,note.stderr)
    out,err=proc.communicate(timeout=15)
    print('CAPTURE:',proc.returncode,out,err)
    th.join(2)
    runs=list(pathlib.Path(d).glob('native-*'));assert len(runs)==1,runs
    combined=(runs[0]/'combined.log').read_text()
    print('COMBINED:',combined)
    assert 'NOTE 6 kurz' in combined
    assert 'GESTURE=PRESS' in combined and 'GESTURE=LONG' in combined and 'GESTURE=LONG_RELEASE' in combined
    assert combined.index('GESTURE=PRESS')<combined.index('NOTE 6 kurz')<combined.index('GESTURE=LONG')
    assert proc.returncode==0
    assert (runs[0]/'debugspi-original.bin').stat().st_size>0
    assert 'hk_received_events=3' in (runs[0]/'report.txt').read_text()
    assert 'NOTE 6 kurz' in out and 'GESTURE=LONG' in out
    assert not (pathlib.Path(d)/'mibr-keypanel-native-current').exists(), 'pointer cleanup failed'
    (pathlib.Path(d)/'mibr-keypanel-native-current').write_text('STALE_SENTINEL\n')
    conflict=subprocess.run([str(exe),'--capture','--out','.','--seconds','10'],cwd=d,capture_output=True,text=True,timeout=3)
    assert conflict.returncode==2 and 'pointer exists' in conflict.stderr,conflict.stderr
    assert (pathlib.Path(d)/'mibr-keypanel-native-current').read_text()=='STALE_SENTINEL\n'
    print('INTEGRATION_TEST=PASS (2 consoles, SD-only pointer, stale-marker guard, split TCP, long events)')