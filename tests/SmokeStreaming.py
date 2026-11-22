#!/usr/bin/env python3
"""Validate FFmpeg WAV-on-stdout -> LAME stdin -> FFmpeg stream-copy remux.
This tests real codec CLI compatibility, NOT the Qt QProcess implementation.
"""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile

for tool in ('ffmpeg', 'ffprobe', 'lame'):
    if not shutil.which(tool):
        raise SystemExit(f'Missing tool: {tool}')

with tempfile.TemporaryDirectory(prefix='lamexp-stream-') as tmp:
    root = Path(tmp)
    source = root / 'Töne - 🎵.flac'
    intermediate = root / 'encoded.mp3'
    target = root / 'Ergebnis - 🎵.mp3'
    subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=880:duration=2',
                    '-metadata','artist=Test Künstler','-metadata','album=Pipeline Test','-c:a','flac',str(source)],check=True)
    # Shell-less OS-level streaming with bounded pipe buffer.
    decoder = subprocess.Popen(['ffmpeg','-hide_banner','-nostdin','-loglevel','error','-i',str(source),
                  '-map','0:a:0','-vn','-sn','-dn','-map_metadata','-1','-c:a','pcm_s16le','-f','wav','pipe:1'],
                  stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    encoder = subprocess.Popen(['lame','--nohist','-q','2','-V','2','-',str(intermediate)],
                                stdin=decoder.stdout, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    decoder.stdout.close()
    _, encode_error = encoder.communicate(timeout=30)
    _, decode_error = decoder.communicate(timeout=30)
    if decoder.returncode or encoder.returncode:
        raise AssertionError(f'Decode {decoder.returncode}: {decode_error!r}; encode {encoder.returncode}: {encode_error!r}')
    subprocess.run(['ffmpeg','-hide_banner','-nostdin','-loglevel','error','-n','-i',str(intermediate),
                    '-i',str(source),'-map','0:a:0','-map_metadata','1','-c:a','copy','-id3v2_version','3',str(target)],check=True)
    result = subprocess.check_output(['ffprobe','-v','error','-show_entries','format_tags=artist,album:stream=codec_name',
                                      '-of','json',str(target)],text=True)
    info = json.loads(result)
    assert info['streams'][0]['codec_name'] == 'mp3', info
    tags = {k.lower(): v for k, v in info['format'].get('tags',{}).items()}
    assert tags['artist'] == 'Test Künstler', info
    assert tags['album'] == 'Pipeline Test', info
    assert target.stat().st_size > 1000
    assert not (root/'decoded.wav').exists()
    print('PASS: streaming PCM, no WAV intermediate, mp3 + Unicode tags')
