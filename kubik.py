"""Kübik API için tek dosyalık Python kütüphanesi (yalnız standart kütüphane).

    from kubik import Kubik
    kubik = Kubik("kbk_ANAHTARIN")
    print(kubik.sor("Merhaba!"))

    sohbet = kubik.sohbet()            # aynı konuşmada devam eder
    print(sohbet.sor("Python'da liste nasıl sıralanır?"))
    print(sohbet.sor("Tersten?"))

Anahtar: Kübik > Profil > API platformu (Max+ ve üstü). Belgeler: https://asrinata.github.io/kubikai/docs.html#api
"""
import json
import time
import urllib.error
import urllib.request

__version__ = '1.0'
ADRES = 'https://kubik-ai.asrinatayilmazok.workers.dev/api/v1'


class KubikHatasi(Exception):
    def __init__(self, durum, mesaj):
        super().__init__(f'{durum}: {mesaj}')
        self.durum, self.mesaj = durum, mesaj


class Kubik:
    def __init__(self, anahtar, adres=ADRES, zaman_asimi=600):
        if not anahtar.startswith('kbk_'):
            raise ValueError('Kübik API anahtarı kbk_ ile başlar.')
        self.anahtar, self.adres, self.zaman_asimi = anahtar, adres.rstrip('/'), zaman_asimi

    def _istek(self, yol, govde=None):
        istek = urllib.request.Request(self.adres + yol, data=json.dumps(govde).encode() if govde is not None else None,
                                       method='POST' if govde is not None else 'GET',
                                       headers={'Authorization': 'Bearer ' + self.anahtar, 'Content-Type': 'application/json',
                                                'User-Agent': f'Mozilla/5.0 (compatible; kubik-py/{__version__})'})
        try:
            with urllib.request.urlopen(istek, timeout=60) as cevap:
                return json.loads(cevap.read())
        except urllib.error.HTTPError as hata:
            try:
                mesaj = json.loads(hata.read()).get('detail', '')
            except ValueError:
                mesaj = hata.reason
            raise KubikHatasi(hata.code, mesaj) from None

    def gonder(self, mesaj, sohbet_id=None, efor='balanced', dil='tr'):
        """İş açar, beklemez. -> {'id', 'conversation_id', 'status'}"""
        return self._istek('/chat', {'message': mesaj, 'conversation_id': sohbet_id, 'effort': efor, 'language': dil})

    def sonuc(self, is_id):
        return self._istek('/chat/' + is_id)

    def bekle(self, is_id, aralik=2):
        bitis = time.monotonic() + self.zaman_asimi
        while time.monotonic() < bitis:
            durum = self.sonuc(is_id)
            if durum['status'] == 'done':
                return durum
            if durum['status'] == 'error':
                raise KubikHatasi('error', durum.get('detail', 'Yanıt üretilemedi.'))
            time.sleep(aralik)
        raise TimeoutError('Kübik zamanında cevap vermedi.')

    def sor(self, mesaj, **ayar):
        """Soruyu gönderir, cevabı bekleyip metnini döndürür."""
        return self.bekle(self.gonder(mesaj, **ayar)['id'])['text']

    def sohbet(self):
        return Sohbet(self)

    def modeller(self):
        return self._istek('/models')['models']


class Sohbet:
    def __init__(self, kubik):
        self.kubik, self.id = kubik, None

    def sor(self, mesaj, **ayar):
        is_ = self.kubik.gonder(mesaj, sohbet_id=self.id, **ayar)
        self.id = is_['conversation_id']
        return self.kubik.bekle(is_['id'])['text']


if __name__ == '__main__':
    import sys
    if len(sys.argv) < 3:
        print('Kullanım: python kubik.py kbk_ANAHTAR "sorun"')
    else:
        print(Kubik(sys.argv[1]).sor(' '.join(sys.argv[2:])))
