"""External-tool smoke check; requires ffmpeg, lame and ffprobe on PATH.

This exercises the command-line codec sequence, not the Qt JobManager.
"""
import shutil
import subprocess
import tempfile
from pathlib import Path


def run(args):
    subprocess.run(args, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def main():
    for program in ('ffmpeg', 'lame', 'ffprobe'):
        if not shutil.which(program):
            raise SystemExit(f'Missing required executable: {program}')
    with tempfile.TemporaryDirectory(prefix='lamexp-ü-') as temp:
        base = Path(temp)
        source = base / 'Eingabe ü.flac'
        wav = base / 'decoded.wav'
        stage = base / '.lamexp-smoke.mp3'
        target = base / 'Ausgabe ö.mp3'
        run(['ffmpeg', '-hide_banner', '-nostdin', '-loglevel', 'error',
             '-f', 'lavfi', '-i', 'sine=frequency=440:duration=0.3',
             '-c:a', 'flac', str(source)])
        run(['ffmpeg', '-hide_banner', '-nostdin', '-loglevel', 'error',
             '-n', '-i', str(source), '-map', '0:a:0', '-vn', '-sn', '-dn',
             '-map_metadata', '-1', '-c:a', 'pcm_s16le', '-f', 'wav', str(wav)])
        run(['lame', '--nohist', '-q', '5', '-V', '4', str(wav), str(stage)])
        assert stage.stat().st_size > 0
        assert not target.exists()
        stage.rename(target)
        probe = subprocess.run(['ffprobe', '-v', 'error', '-show_entries',
                                'stream=codec_name', '-of', 'default=noprint_wrappers=1',
                                str(target)], capture_output=True, text=True, check=True)
        assert 'codec_name=mp3' in probe.stdout
    print('External codec smoke check passed')


if __name__ == '__main__':
    main()
