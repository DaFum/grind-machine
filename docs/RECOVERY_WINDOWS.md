# FM-1 unter Windows wiederherstellen (Boot-Modus, ohne Zusatzhardware)

Diese Anleitung ist die Windows-Fassung von [RECOVERY.md](RECOVERY.md) (dort steht die Mac-Fassung und die
Erklärung, wie es funktioniert). Sie gilt, wenn der Web-Installer **und** M-UPGRADE den FM-1 nicht mehr zu Ende
beschreiben, zum Beispiel wenn M-UPGRADE bei "Resuming OTA recovery" immer an derselben Stelle mit
`KS MIDI receive timeout error=258` abbricht. Sie spielt M-VAVEs Original-Firmware (V15) über den Boot-Modus des
Chips zurück. Der Boot-Modus liegt im Chip selbst und lässt sich von keiner Firmware löschen.

**Auf eigenes Risiko.** Lies jeden Schritt, bevor du ihn ausführst, und brich beim ersten Schritt ab, der nicht so
ausgeht wie beschrieben. Das Werkzeug liest und schreibt nur den FM-1. Der Boot-Bereich, der Chip-Schlüssel und
der Datenbereich (Einstellungen, Samples) werden nie beschrieben.

**Stand der Prüfung:** Das Werkzeug ist gegen einen simulierten FM-1 getestet (`tests/fm1_uboot_test.py`) und lief
einmal auf einem echten FM-1 von einem Mac aus. Der Windows-Weg (WinUSB-Treiber, libusb-DLL) ist **nicht** an
echter Hardware geprüft. Wenn ein Schritt anders ausgeht, als hier steht, stopp und melde es.

## Was du brauchst

- Windows 10 oder 11 (64 Bit), ein USB-Datenkabel, das mit dem FM-1 schon funktioniert hat, direkt am PC (kein Hub).
- [Python 3](https://www.python.org/downloads/) (64 Bit) mit dem Startprogramm `py`.
- [Git](https://git-scm.com/download/win) (oder die ZIP-Datei dieses Repositories).
- [Zadig](https://zadig.akeo.ie/) (installiert den WinUSB-Treiber für das Gerät).
- Die 64-Bit-`libusb-1.0.dll` aus den Releases von <https://github.com/libusb/libusb/releases> (Datei
  `libusb-*.7z`, darin `VS2022\MS64\dll\libusb-1.0.dll`; mit 7-Zip entpacken).
- M-VAVEs Original-Firmware `FM-1.fwsc` (V15) von <https://www.m-vave.com/download> (Downloads, PC Firmware,
  FM-1 V15).
- Ein geladener FM-1.

Alle Befehle sind für **PowerShell** und werden im Ordner dieses Repositories ausgeführt (`cd` dorthin).

## 0. Einmalig vorbereiten

```powershell
py -3 -m venv $HOME\fm1-rescue
& $HOME\fm1-rescue\Scripts\pip install pyusb
git clone https://github.com/kagaimiq/jl-uboot-tool $HOME\jl-uboot-tool
Get-Item $HOME\jl-uboot-tool\data\loaderblobs\usb\wl82loader.bin      # Länge 24064
Get-FileHash -Algorithm SHA256 $HOME\Downloads\FM-1.fwsc            # muss mit db1642b2 beginnen
```

Das Werkzeug lehnt jede andere Firmware-Datei ab. Lege die entpackte `libusb-1.0.dll` zum Beispiel nach
`C:\fm1-rescue\libusb-1.0.dll` und kürze die Befehle ab:

```powershell
$env:FM1_LIBUSB = "C:\fm1-rescue\libusb-1.0.dll"
$RESCUE = "$HOME\fm1-rescue\Scripts\python.exe tools\rescue\fm1_uboot.py"
$LOADER = "$HOME\jl-uboot-tool\data\loaderblobs\usb\wl82loader.bin"
```

(`$env:FM1_LIBUSB` und die beiden Variablen gelten nur für dieses PowerShell-Fenster. Öffnest du ein neues,
setze sie neu.)

## 1. FM-1 in den Boot-Modus bringen

Nimm eine dieser Möglichkeiten:

- **Die Firmware läuft** (Hortator, Felucca oder GRIND): **OCT−** und **OCT+** zusammen halten. Nach 2 s erscheint
  ein Countdown, Loslassen bricht ab. Bei 5 s zeigt der Bildschirm `UBOOT`.
- **Vom Rechner:** die SysEx-Nachricht `F0 22 24 35 7D F7` an den MIDI-Port des FM-1 senden (geht auch bei
  M-VAVEs V15). Ein Windows-Programm dafür ist zum Beispiel MIDI-OX; auf diesem Weg habe ich es nicht geprüft.
- **Die Firmware stürzt direkt nach dem Start ab:** zwei Abstürze innerhalb von 30 s nach dem Start führen von selbst
  in den Boot-Modus (Felucca-basierte Firmware).

Der Bildschirm wird schwarz oder behält das letzte Bild. Der FM-1 bleibt im Boot-Modus, bis du ihn ausschaltest oder
das Werkzeug fertig ist. Prüfe im **Geräte-Manager**: unter "Laufwerke", "USB-Massenspeichergeräte" oder
"Andere Geräte" sollte ein Gerät **`WL80UBOOT1.00`** (USB-ID `4C4A:8057`) auftauchen. Schließe vorher M-UPGRADE, den
Web-Installer und alle Programme, die den FM-1 per MIDI benutzen.

Meldet sich der FM-1 weder normal noch als `WL80UBOOT`, hilft diese Anleitung nicht. Dann bleibt der
[FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter) (ein Seeed XIAO RP2040 an den USB-Leitungen des
FM-1), der den Boot-Modus auch ohne laufende Firmware erzwingt.

## 2. Treiber für das Boot-Modus-Gerät (einmalig)

Windows lädt für das Gerät seinen Massenspeicher-Treiber. Das Werkzeug braucht stattdessen **WinUSB**:

1. FM-1 im Boot-Modus am PC lassen (Schritt 1), Zadig starten.
2. Menü **Options → List All Devices** einschalten.
3. In der Liste das Gerät mit der USB-ID **`4C4A 8057`** wählen (Name `WL80UBOOT1.00` oder ähnlich). **Wähle kein
   anderes Gerät**: Zadig ersetzt den Treiber des gewählten Geräts.
4. Rechts als Ziel-Treiber **WinUSB** einstellen und auf **Replace Driver** klicken.

Danach ist das Gerät für das Werkzeug erreichbar. Der Treiber bleibt für diese USB-ID erhalten. Willst du später
wieder einen Laufwerksbuchstaben für das Boot-Modus-Gerät, mache es im Geräte-Manager über "Treiber → Zurücksetzen"
rückgängig. Der normale FM-1-Betrieb (MIDI, Installer) ist davon nicht betroffen.

## 3. Den ganzen Flash sichern (nur Lesen)

```powershell
Invoke-Expression "$RESCUE backup --loader $LOADER $HOME\fm1-backup.bin"
```

Erwartete Ausgabe:

```
chip key 0x980F, flash type 3 id 0x856014
read 1: 2.3 s, sha256 …
read 2: 2.3 s, sha256 …   (dieselbe Summe wie read 1)
saved C:\Users\du\fm1-backup.bin; nothing was written to the FM-1
```

- `fm1_uboot: libusb not found`: `$env:FM1_LIBUSB` zeigt nicht auf die DLL (oder sie ist 32 Bit, Python aber 64 Bit).
- `no WL80UBOOT (4C4A:8057) device`: der FM-1 ist nicht im Boot-Modus, oder Zadig hat nicht das richtige Gerät
  bekommen (Schritt 2).
- `cannot take the device …`: der WinUSB-Treiber fehlt (Schritt 2).
- `not an FM-1 as expected`: hier stoppen. Es wurde nichts gelesen oder geschrieben.
- `the two reads differ`: noch einmal versuchen, bei Wiederholung mit einem anderen Kabel.

Nach dem Lauf startet der FM-1 neu und bootet seine Firmware wie zuvor. Hebe `fm1-backup.bin` an einem sicheren Ort
auf: es ist der Flash dieses FM-1 samt seinem Datenbereich.

## 4. Die Wiederherstellung prüfen (schreibt nichts)

FM-1 wieder in den Boot-Modus bringen (Schritt 1), dann:

```powershell
Invoke-Expression "$RESCUE restore --loader $LOADER $HOME\Downloads\FM-1.fwsc $HOME\fm1-backup.bin"
```

Erwartet: wieder die Chip-Zeile, `143 of 143 app sectors differ from stock V15` (weniger, wenn ein Teil schon Original
ist) und `DRY RUN: nothing written (add --write)`.

Die Prüfung lehnt ab, wenn:

- **das Paket nicht V15 ist** (`is not M-VAVE's stock V15 FM-1.fwsc`): Datei bei M-VAVE neu laden.
- **das Backup nicht zu diesem FM-1 passt** (`head or data differ from the backup`): neues Backup machen (Schritt 3).

## 5. Original-Firmware schreiben

FM-1 wieder in den Boot-Modus bringen (Schritt 1), dann:

```powershell
Invoke-Expression "$RESCUE restore --loader $LOADER $HOME\Downloads\FM-1.fwsc $HOME\fm1-backup.bin --write"
```

Kabel nicht ziehen und den FM-1 nicht ausschalten, bis `done` erscheint (etwa 15 s). Alle 16 Sektoren kommt eine
Fortschrittszeile, am Ende:

```
done: stock V15 app written and verified. Unplug USB or power-cycle the FM-1.
```

- `verify failed, again`: ein Wiederholungsversuch, du musst nichts tun.
- `failed 3 times: STOP` oder `FINAL CHECK FAILED`: den FM-1 angeschaltet und im Boot-Modus lassen und denselben
  Befehl noch einmal ausführen (ist er stehen geblieben, vorher wieder in den Boot-Modus bringen). Es werden nur
  die Sektoren geschrieben, die noch nicht Original sind, ein erneuter Lauf ist also sicher. Dasselbe gilt für
  einen abgebrochenen Lauf, zum Beispiel bei gezogenem Kabel.

## 6. FM-1 starten und die Firmware neu installieren

1. USB abziehen oder den FM-1 aus- und einschalten. Er startet mit M-VAVEs Original-Firmware.
2. Danach die gewünschte Firmware wieder über den Web-Installer aufspielen:
   <https://dafum.github.io/grind-machine/webapp/installer/> (Chrome oder Edge, per USB).

Der Datenbereich oberhalb von `0x93000` (Einstellungen, Presets, eigene Samples) wurde nicht angefasst: er ist so,
wie er in deinem Backup steht.
