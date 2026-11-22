"""External codec integration (not a Qt/Windows executable test).
Exercises an MP3 input with ID3 metadata and embedded cover through FFmpeg -> WAV -> LAME -> FFmpeg stream-copy remux.
"""
import json
import shutil
import subprocess
import tempfile
from pathlib import Path


def run(*args):
    return subprocess.run(args, check=True, capture_output=True, text=True)


def main():
    for name in ('ffmpeg', 'ffprobe', 'lame'):
        if not shutil.which(name):
            raise SystemExit(f'{name} not installed')
    with tempfile.TemporaryDirectory(prefix='next-ä-') as d:
        p=Path(d)
        source=p/'Quelle ü.mp3'; cover=p/'cover.jpg'; pcm=p/'audio.wav'
        encoded=p/'encoded.mp3'; output=p/'Ausgabe ä.mp3'
        run('ffmpeg','-hide_banner','-nostdin','-loglevel','error','-f','lavfi',
            '-i','color=blue:s=64x64:d=1','-frames:v','1',str(cover))
        run('ffmpeg','-hide_banner','-nostdin','-loglevel','error','-f','lavfi',
            '-i','sine=frequency=440:duration=1', '-i',str(cover),
            '-map','0:a:0','-map','1:v:0','-c:a','libmp3lame','-q:a','4',
            '-c:v','mjpeg','-disposition:v:0','attached_pic',
            '-metadata','artist=Björk','-metadata','album=Testalbum',
            '-metadata','title=Prüfung','-metadata','track=3',
            '-id3v2_version','3',str(source))
        run('ffmpeg','-hide_banner','-nostdin','-loglevel','error','-n','-i',str(source),
            '-map','0:a:0','-vn','-sn','-dn','-map_metadata','-1','-c:a','pcm_s16le','-f','wav',str(pcm))
        run('lame','--nohist','-q','5','-V','4',str(pcm),str(encoded))
        run('ffmpeg','-hide_banner','-nostdin','-loglevel','error','-n',
            '-i',str(encoded),'-i',str(source),'-map','0:a:0','-map_metadata','1',
            '-c:a','copy','-map','1:v:0','-c:v','mjpeg','-disposition:v:0','attached_pic',
            '-id3v2_version','3',str(output))
        data=json.loads(run('ffprobe','-v','error','-show_format','-show_streams',
                            '-of','json',str(output)).stdout)
        tags={k.lower():v for k,v in data['format'].get('tags',{}).items()}
        assert tags['artist']=='Björk',tags
        assert tags['album']=='Testalbum',tags
        assert tags['title']=='Prüfung',tags
        assert any(s.get('codec_name')=='mp3' for s in data['streams'])
        assert any(s.get('disposition',{}).get('attached_pic')==1 for s in data['streams'])
        assert output.stat().st_size>1000
        print('Metadata + attached cover integration passed')
if __name__=='__main__': main()
