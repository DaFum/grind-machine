# GRIND MACHINE — Benutzerhandbuch

| Merkmal | Stand |
| --- | --- |
| Ausgabe | 1.5 des Handbuchs, 28. November 2026 |
| Gültig für | M-VAVE FM-1, GRIND MACHINE mit `GRIND_NATIVE=1`, Vorschau `grind-v0.1.0-preview.7`, Quellstand `c9857e4` |
| Zielgruppe | Musikerinnen und Musiker, die Drum-Patterns erzeugen, bearbeiten und live wechseln möchten |
| Zeitbedarf | Der Einstieg hängt davon ab, ob bereits ein passender Simulator oder Build vorliegt |

GRIND MACHINE ist ein Fork von Hortator für den M-VAVE FM-1. Der GRIND-Generator schreibt Drum-Patterns in die acht Spuren des Sequenzers. Du kannst danach einzelne Schritte bearbeiten, Treffer und Pausen vor weiteren Änderungen schützen und das Ergebnis als Projekt speichern.

**Zum ersten Beat** · Im Simulator hörst du den erzeugten Rhythmus. Im USB-Webeditor steuerst du den angeschlossenen FM-1. Die Editor-Demo erlaubt dir, die Oberfläche ohne Gerät auszuprobieren.

## Den Simulator starten

1. Öffne den [GRIND MACHINE Simulator](https://dafum.github.io/grind-machine/#play).
2. Schalte **Power** ein.
3. Drücke **ARP** nach den beiden GRIDS-Seiten weiter, bis die GRIND-Regler erscheinen.
4. Starte mit **PLAY** oder der Leertaste die Wiedergabe.

Ein frisches Browserprofil enthält Mathcore 7/4 in Demo-Slot `D`. **Controls** öffnet die Maus-, Touch- und Tastaturhilfe. Regler kannst du ziehen oder mit dem Mausrad drehen. Für Tastenkombinationen arretiert ein Rechtsklick oder langes Drücken eine Schaltfläche. Erneutes Antippen löst sie.

**Reset to demos** löscht die Spielstände des aktuellen Browsers und stellt die vier Demos wieder her.

### Lokalen Simulator bauen

Verwende diesen Ablauf, wenn du eine Kopie für einen bestimmten Quellstand brauchst. Für den lokalen Browser-Build brauchst du Git, Python 3 mit Pillow, einen C-Compiler, Docker und Node.js ab Version 18.

1. Klone das Repository.

   ```sh
   git clone https://github.com/DaFum/grind-machine.git
   ```

2. Wechsle in das Verzeichnis `grind-machine`.
3. Checke den in diesem Handbuch beschriebenen Stand aus.

   ```sh
   git checkout --detach c9857e49d2270df0eb44052097bfafc39afa5286
   ```

4. Baue den Websimulator mit aktiviertem GRIND-Generator.

   ```sh
   GRIND_NATIVE=1 bash tools/build_sim.sh --preview
   ```

5. Starte den lokalen Webserver.

   ```sh
   python3 -m http.server 8001 --bind 127.0.0.1 --directory build/sim/preview
   ```

6. Öffne [den Simulator](http://127.0.0.1:8001/) im Browser.
7. Betätige **ARP** wiederholt, bis `STYLE`, `AGGR`, `DENS` und `CHAOS` erscheinen.

Der Browser speichert Projekte in IndexedDB. Verwende beim nächsten Start dieselbe Browseradresse und dasselbe Browserprofil. USB, MIDI-Anschlüsse und Firmware-Updates erfordern ein echtes Gerät. [Simulator-Dokumentation](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/SIMULATOR.md)

## BLAST IGNITION ansehen

Beim Einschalten zeigt der FM-1 während der Initialisierung `SYSTEM WAKE`. Auf der ersten regulären Anzeige folgt eine rund 400 ms lange Startanimation. Eingaben beenden die Animation und lösen die zugehörige Bedienaktion aus.

Öffne die [BLAST-IGNITION-Vorschau](https://dafum.github.io/grind-machine/?boot=1#play) im Browser. Sie zeigt die Szene ohne Zugriff auf die Browser-Spielstände. **Esc** führt zur Demo-Schleife zurück. Bei reduzierter Bewegung zeigt die Vorschau das abschließende `READY`-Bild. [Simulator-Dokumentation](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/manual/15-simulator.md)

## Einen D-Beat erzeugen

Verwende ein Testprojekt oder speichere dein aktuelles Projekt vorab. Eine neue GRIND-Erzeugung kann die Schritte und das Raster aller acht Spuren ersetzen.

1. Stoppe den Transport mit **PLAY**, falls er läuft.
2. Tippe **ARP**, bis die GRIND-Seite mit `STYLE` erscheint.
3. Drehe **KNOB 1** bei `STYLE` auf `DBET`.
4. Tippe **ARP**, um die Seite mit `KIT` zu öffnen.
5. Drehe **KNOB 4** bei `KIT` einen Rastschritt nach rechts.

   Das Display zeigt `AGAIN: ROLE KIT`. Die Aktion ersetzt die Modelle und Klangparameter der acht Spuren.

6. Drehe **KNOB 4** innerhalb von rund 1,5 Sekunden erneut nach rechts.

   `ROLE KIT SET` bestätigt die Zuordnung. Pegel, Panorama, Effekt-Sends und die Sequenzschritte bleiben erhalten.

7. Drücke **PLAY**.

Der D-Beat läuft mit den Klängen des Rollen-Kits. Auf **SEQ** siehst du die aktiven Schritte der gewählten Spur. Mit **ALGORITHM** wechselst du die Spur. Im Simulator bewegst du einen Regler per Ziehen oder Mausrad. [GRIND-Bedienlogik](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/firmware/src/grind/ui.c)

## Einen Treffer oder eine Pause schützen

Bearbeite zunächst das Pattern, das du variieren möchtest. Du kannst mit **SEQ** einen Schritt durch Antippen seiner weißen Taste ein- oder ausschalten.

1. Wähle die Spur mit **ALGORITHM**.
2. Öffne mit **ARP** die GRIND-Seite mit `KEEP`.
3. Tippe **ENV**, falls `STEPS` noch aus ist. Das Schloss im Display zeigt den Modus an.
4. Halte die weiße Taste des gewünschten Schritts, bis `STEP … KEPT` erscheint.
5. Öffne mit **ARP** die GRIND-Seite mit `MUT`.
6. Drehe **KNOB 2** bei `MUT`, um eine Variation zu erzeugen.

Der geschützte Schritt behält seinen Zustand, auch wenn er eine Pause war. Tippe **ENV** erneut, um `STEPS` zu verlassen. Für Schritte in einer anderen Bank halte **ENV** und drücke **OCT+** oder **OCT-**.

| Aufgabe | Bedienung auf der Seite mit `KEEP` | Anzeige |
| --- | --- | --- |
| Ganze Spur schützen | **KNOB 1** nach rechts drehen | `TRACK KEPT` |
| Schutz der gewählten Spur aufheben | **KNOB 1** nach links drehen | `TRACK FREE` |
| Schutz aller Spuren aufheben | **KNOB 3** bei `FREE` nach rechts drehen | `ALL FREE` |
| Letzte GRIND-Änderung umkehren | **KNOB 2** bei `UNDO` nach rechts drehen | `UNDONE` |
| GRIND-Änderung wiederherstellen | **KNOB 2** bei `UNDO` erneut nach rechts drehen | `REDONE` |

`UNDO` stellt den Pattern-Zustand vor der letzten GRIND-Änderung samt Spurlängen wieder her. Sichere spätere Handarbeit vorher, wenn du sie behalten möchtest. `KEEP` gilt bis zum Laden oder Einwechseln eines anderen Patterns und wird beim Speichern nicht mitgesichert.

## Ein Pattern während der Wiedergabe wechseln

1. Starte ein einzelnes Pattern mit **PLAY**. Beende vorher eine laufende **SONG**-Kette.
2. Öffne mit **ARP** die GRIND-Seite mit `WHEN`.
3. Wähle mit **KNOB 3** den Umschaltpunkt.
4. Ändere `MUT` oder kehre zur Seite mit `STYLE` zurück und wähle einen anderen Stil.

Bei `STEP` beginnt das neue Pattern am nächsten Schritt. Bei `END` wartet es bis zum Ende der längsten laufenden Spur. Beim Wechsel beginnen alle acht Spuren gemeinsam bei Schritt 1. Im Stillstand wird ein gültiges Pattern sofort übernommen. Eine neuere Erzeugung ersetzt eine noch wartende. Stoppen verwirft eine ausstehende Änderung. Änderst du Schritte, `LEN`, `DIV`, `SWG` oder `SRC`, kann die wartende Erzeugung verworfen werden. Änderst du `KEEP` nach dem Vorbereiten eines Patterns, verwirft die Firmware dieses Pattern. Erzeuge es erneut, damit der neue Schutz berücksichtigt wird. [Tests für Pattern-Wechsel](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/tests/grind/test_cue.c)

## Ein Projekt speichern und prüfen

Wähle einen freien Slot oder einen Slot, dessen gespeicherten Inhalt du ersetzen möchtest.

1. Stoppe den Transport.
2. Tippe **SAVE**, bis `PROJECT` erscheint.
3. Wähle mit **KNOB 1** bei `SLOT` einen der vier Slots `A` bis `D`.
4. Drehe **KNOB 4** bei `SAVE` einen Rastschritt nach rechts.
5. Drehe **KNOB 4** innerhalb von rund 1,5 Sekunden erneut nach rechts.
6. Reagiere bei `OVERWRITE …?` mit **OCT+**, wenn du die Änderungen im Zielslot überschreiben möchtest. Mit **OCT-** brichst du ab.
7. Prüfe, ob `SAVED` erscheint.

Zum Laden verwendest du auf `PROJECT` **KNOB 3** bei `LOAD` mit derselben doppelten Drehung. Bei `DISCARD EDITS?` verwirft **OCT+** die ungespeicherten Änderungen des gewählten Slots. Der letzte erfolgreich gespeicherte Slot wird beim normalen Start geladen. `SAVED (RAM)` bedeutet, dass die Änderungen nur im Arbeitsspeicher vorliegen.

Das Projekt speichert das erzeugte musikalische Ergebnis. `STYLE` und `WHEN` gehören zu den Geräteeinstellungen. Beim nächsten Start kommen die übrigen Generatorregler mit den Werkswerten des Stils zurück, `MUT` beginnt bei 0. [Projektspeicher](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/manual/06-projects.md)

## Bedienreferenz

| GRIND-Seite | Regler | Anzeige | Funktion |
| --- | --- | --- | --- |
| Erste | 1 | `STYLE` | Einer von 23 Stilen. Setzt dessen Werkswerte und `MUT` auf 0. |
| Erste | 2 | `AGGR` | Aggressivität, 0–100 %. |
| Erste | 3 | `DENS` | Trefferdichte, 0–100 %. |
| Erste | 4 | `CHAOS` | Rhythmische Unregelmäßigkeit, 0–100 %. |
| Zweite | 1 | `HUMAN` | Menschlich wirkende Timing-Abweichungen, 0–100 %. |
| Zweite | 2 | `MUT` | Variationsnummer, 0–99. Erzeugt eine andere Fassung des Stils. |
| Zweite | 3 | `WHEN` | Übernahme bei `STEP` oder `END`. |
| Zweite | 4 | `KIT` | `ROLE` weist die passenden Klänge zu. Bestätigung und Stillstand erforderlich. |
| Dritte | 1 | `KEEP` | Zahl geschützter Schritte der gewählten Spur. |
| Dritte | 2 | `UNDO` | Wechselt zwischen dem Zustand vor und nach der letzten GRIND-Änderung. |
| Dritte | 3 | `FREE` | Hebt mit `ALL` alle Schrittmarkierungen auf. |
| Dritte | 4 | `VARY` | Anschlagvariation während der Wiedergabe, 0-100. |
| Vierte | 1-3 | `SINK`, `SWELL`, `CUT` | Live-Effekte und Performance-Stopp. |
| Fünfte | 1-3 | `STYLE`, `BANK`, `APPLY` | Stil und Bank vor der Übernahme ansehen. |
| Sechste | 1-4 | `RHY`, `TIME`, `TMBR`, `MUT` | Ebenen der gewählten Spur schützen und variieren. |

| Spur | Rolle | Spur | Rolle |
| --- | --- | --- | --- |
| T1 | Kick | T5 | Crash |
| T2 | Snare | T6 | China |
| T3 | Hi-Hat | T7 | Hohes Tom |
| T4 | Ride | T8 | Tiefes Tom |

### Die 23 Stile

| Display | Stil | Display | Stil |
| --- | --- | --- | --- |
| `TRAD` | Traditional | `ABRP` | Abrupt Break |
| `HAMR` | Hammer | `STOP` | Stop-and-Go |
| `BOMB` | Bomb | `BRKD` | Breakdown |
| `GRAV` | Gravity | `2STP` | Two-Step |
| `BLST` | Grind Blast | `HALF` | Half-Time |
| `DBET` | D-Beat | `CHAO` | Chaos |
| `SKNK` | Skank | `INDU` | Industrial |
| `2XBS` | Doublebass | `POLY` | Polymetric |
| `TECH` | Technical | `SLDG` | Sludge |
| `BRUT` | Brutal | `FUNR` | Funeral |
| `FSLO` | Fast-Slow | `STNR` | Stoner |
| `MATH` | Mathcore 7/4 | | |

Bei `FSLO` zeigt die Werksreferenz ein Pattern mit 32 Schritten statt 16. Bei `POLY` laufen im Werksmuster Snare und China mit 12 beziehungsweise 15 Schritten, die übrigen Spuren mit 16. [Stilreferenz aus den Testdaten](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/tests/grind/golden/styles-v1.csv)

## Einen Stil ansehen und als Pattern übernehmen

Die fünfte GRIND-Seite zeigt das Raster als visuelle Vorschau. `STYLE` auf dieser Seite allein ändert das laufende Pattern nicht.

1. Drücke **ARP**, bis `STYLE`, `BANK` und `APPLY` erscheinen.
2. Wähle mit **KNOB 1** den Stil. `MATH` öffnet Mathcore 7/4.
3. Blättere mit **KNOB 2** in 16-Schritt-Bänken durch längere Patterns.
4. Prüfe das Vorschauraster. Es zeigt die acht Rollen und berücksichtigt `KEEP`.
5. Drehe **KNOB 3** bei `APPLY` nach rechts auf `OK`, um das Pattern zu übernehmen.

Im Stillstand erfolgt die Übernahme sofort. Während der Wiedergabe gilt `WHEN` von Seite 2. Bei `PREVIEW WAITS` warte auf die Vorschau. Bei `KEEP DOES NOT FIT` prüfe den Schrittschutz. Verlässt du die Seite vor `APPLY`, bleibt das aktuelle Pattern aktiv.

**Mathcore 7/4** basiert auf Technical und gruppiert sieben Viertel als 3+2+2. Ein Takt dauert bei Sechzehnteln 28 Schritte. Der nächste Takt beginnt nach Schritt 28. Das Referenzpattern setzt die betonten Kick-Schläge auf 1, 13 und 21, die Snare auf 9, 17 und 25. [GRIND-Seiten](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/grind/live-pages.md)

Ein 7/4-Takt umfasst bei `1/8` 14 Schritte, bei `1/16` 28 und bei `1/32` 56. Bei `1/16` passen zwei Takte in 56 Schritte. Prüfe `LEN` und `DIV` auf der Seite **PATTERN**, wenn du ein gespeichertes Mathcore-Pattern öffnest. Die 22 Stile der Preview .1 stehen weiterhin in der Stilliste. `MATH` kam in Preview .5 als 23. Eintrag hinzu. Preview .2 bis .4 lesen die gespeicherte MATH-Stilwahl als `STYLE 1`. Preview .1 speichert die Stilwahl noch nicht. Die Stilwahl ist eine Geräteeinstellung. Das erzeugte Pattern speicherst du separat im Projekt. [Changelog](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/CHANGELOG.md#unreleased)

## Eine einzelne Spur variieren

Die erste GRIND-Seite folgt auf zwei GRIDS-Seiten. Deshalb zeigt die Fußzeile für die sechste GRIND-Seite `8/8`. Diese Seite arbeitet nur auf der mit **ALGORITHM** gewählten Spur. Sie unterscheidet Rhythmus und Mikrotiming. `KEEP` und Schritte mit Parameter-Lock- oder MOTION-Ereignissen bleiben bei dieser Mutation geschützt.

1. Wähle mit **ALGORITHM** die Zielspur.
2. Öffne mit **ARP** die Seite mit `RHY`, `TIME`, `TMBR` und `MUT`.
3. Setze `RHY` mit **KNOB 1** auf `LOCK`, wenn Treffer, Akzente, Ratchets und Noten erhalten bleiben sollen.
4. Lass `TIME` mit **KNOB 2** auf `FREE`, wenn sich das Mikrotiming ändern darf.
5. Drehe **KNOB 4** bei `MUT` einen Rasterpunkt nach rechts.

Im Stillstand kann die Mutation sofort übernommen werden. Während der Wiedergabe zeigt `PENDING END`, dass sie bis zum Ende des längsten Patterns wartet. `MUTATED` bestätigt die Übernahme. `LOCKED` bedeutet, dass alle angeforderten Ebenen gesperrt sind. `TIMBRE SOON` signalisiert eine nicht unterstützte Mutation. `TMBR` reserviert einen Schutzschalter, die Klangmutation ist in dieser Preview noch nicht verfügbar.

Ebenensperren bleiben bei Projektwechseln in derselben Sitzung erhalten. Speichere das resultierende Pattern mit **SAVE**. Die Sperrschalter selbst sind keine Projektdaten. [Verhalten der Spurmutation](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/firmware/src/grind/be_runtime.c)

## Die Wiedergabe live formen

| Regler | Wirkung |
| --- | --- |
| `VARY` · Seite 3 | Variiert die Anschlagstärke regulärer Drum-Steps und Ratchets, auch bei Handprogrammierung und `KEEP`. 100 erlaubt bis zu ±8, 0 deaktiviert die Variation. Die Steps bleiben unverändert. |
| `SINK` · Seite 4 | Schiebt den Masterfilter in Richtung Tiefpass. |
| `SWELL` · Seite 4 | Erhöht die Delay- und Hall-Sends. |
| `CUT` · Seite 4 | Stoppt die normale Wiedergabe, kappt die Stimmen und verwirft wartende Änderungen. Delay und Hall klingen aus. Mit **PLAY** startest du wieder. |

Die Wiedergabeeffekte `VARY`, `SINK` und `SWELL` werden nicht im Projekt gespeichert. [Live-Regler](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/grind/live-pages.md)

## Weitere Funktionen des Sequenzers

| Aufgabe | Einstieg | Was du dort einstellst |
| --- | --- | --- |
| Schritte programmieren | **SEQ** öffnen, Spur mit **ALGORITHM** wählen | Weiße Tasten für Treffer. **OCT-** und **OCT+** für Bänke zu je 16 Schritten, bis zu 64 Schritte pro Spur. |
| Treffer verfeinern | Auf `STEP` eine Schritttaste halten | **KNOB 1–4** für `PROB`, `RATCH`, `ACCENT`, `NUDGE`. |
| Reglerwert je Schritt ändern | Auf einer Klang- oder Effektseite **ENV** aktivieren | Schritttaste halten, unterstützten Regler drehen. `LOCK STEP …` zeigt den gehaltenen Schritt an. |
| Einspielen und aufnehmen | Auf `TRACKS` **REC** drücken | Ausgewählte Spur scharf schalten, dann Wiedergabe mit **PLAY** starten. Ein **REC**-Tipp auf einer anderen Seite aktiviert in der Regel `REC ALL`. |
| Noten spielen | **SCL** auf dem FM-1, **SEL** im Simulator | `KEYS` spielt die gewählte Spur tonal. Für Step-Recording bei gestopptem Transport auf `STEP` **REC** drücken. |
| Klang auswählen | **EDIT** oder **PRESETS** | Drum-Modelle, SAMPLE und ACID sowie Presets der gewählten Spur. |
| Klang formen | **FX**, **LFO** oder **GLO** | Spurfilter und Effekte, Modulation, globale Effekte. **FX** halten für `PERFORM`. |
| Spuren abhören | **GLO** halten | Weiße Tasten schalten Spuren stumm, schwarze Tasten schalten sie solo. |
| Song oder Varianten nutzen | **SEQ** bis `SONG`, **SAVE** halten für die Cue-Ansicht | Projekte verketten oder Varianten im laufenden Betrieb einwechseln. |

Im USB-Webeditor kannst du auch Klangparameter, Presets, Projekte, Schritte und Sample-Sets bearbeiten. Eine Sample-Sicherung als `.fsmp` ist ein eigener Arbeitsgang. Ein Projekt-Backup liest den gespeicherten Stand. [Hortator-Webeditor und geerbte Funktionen](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/manual/14-web-editor.md)

### Zwei gespeicherte Projekte als Song spielen

Stoppe die Wiedergabe. Verwende zwei belegte Slots, beispielsweise A und B. Drücke **SEQ** bis **SONG 4/4**. Wähle mit **KNOB 1** die Position `+`, mit **KNOB 2** Slot A und mit **KNOB 3** zwei Wiederholungen. Wähle erneut `+` und füge B mit einer Wiederholung hinzu. Stelle **LOOP** mit **KNOB 4** auf **OFF**. Drücke auf der SONG-Seite **PLAY**. Du hörst A zweimal und danach B einmal, jeweils mit Variation 1. Hat eine vorhandene Kette bereits Zeilen, werden die neuen Zeilen angehängt.

### Spieltiming und Reglerbewegungen aufnehmen

Für eingespielte Noten wähle vor der Aufnahme unter **GLO → GLOBAL 3/7** mit **KNOB 4** das Raster **QUANT**. **1/16** legt jeden Treffer auf den nächstgelegenen Schritt: auf den laufenden, solange der Treffer vor dessen Mitte kommt, sonst auf den folgenden. **OFF** erhält den zeitlichen Versatz als **NUDGE**. Spiele denselben Rhythmus mit beiden Einstellungen ein und vergleiche die versetzten Schritte auf **STEP**.

Für eine Reglerfahrt wähle auf **TRACKS** mit **ALGORITHM** eine hörbare Spur. Drücke **REC**, starte mit **PLAY** und drehe auf **LFO 1/4** den Regler **RATE** des ersten LFO. Stoppe nach einem Durchlauf. Öffne mit **SEQ** die Seite **MOTION 3/4**. Der Zähler **MOTION** zeigt aufgezeichnete Ereignisse. Mit **KNOB 1** schaltest du ihre Wiedergabe aus und wieder ein. Für einen hörbaren Vergleich weise dem LFO wie unten beschrieben ein Ziel zu.

### Klang schichten, modulieren und absenken

| Aufgabe | Bedienung | Ergebnis prüfen |
| --- | --- | --- |
| Sample unter einen Drum-Sound legen | Wähle die Spur mit **ALGORITHM**. Bei ausgeschaltetem **KEYS** drücke **EDIT**, dann **OCT+**. Wähle auf **LAYER 1/2** ein belegtes **SET**, einen **SLOT** und erhöhe **LEVEL**. | Spiele einen Treffer. Bei **LEVEL 0** hörst du das Modell allein. **OCT-** führt zurück. |
| Stereo-Panorama bewegen | Öffne mit **LFO** die Seite **DEST 4/4**. Wähle mit **KNOB 1–4** nacheinander einen freien Slot, **LFO 1**, **PAN** und eine Tiefe über null. | Spiele die Spur in Stereo. Erhöhe **DEPTH** und vergleiche die Bewegung mit **DEPTH 0**. |
| Eine Spur bei jedem Kick absenken | Spiele ein Pattern mit Kick und einer anhaltenden zweiten Spur. Gehe von **HOME** mit **SELECT** zu **COMP 2/3**. Wähle unter **SRC** die Kickspur. Aktiviere mit der weißen Taste der zweiten Spur deren **DUCK**. | Senke **THRSH**, bis die zweite Spur bei jeder Kick kurz leiser wird. Schalte **DUCK** zum Vergleich wieder aus. |

## GRIND im USB-Webeditor bedienen

### Den FM-1 verbinden

Du brauchst einen FM-1 mit GRIND-kompatibler Firmware, ein USB-Datenkabel und einen Browser mit Web MIDI und SysEx. Chrome oder Edge auf einem Computer sind die einfachste Wahl. Mit der [Editor-Demo](https://dafum.github.io/grind-machine/webapp/editor/?mock) kannst du die Oberfläche zuerst ohne Gerät ausprobieren. Der dort angezeigte FM-1 ist eine Simulation.

1. Verbinde den FM-1 per USB mit dem Computer.
2. Öffne den [GRIND MACHINE Editor](https://dafum.github.io/grind-machine/webapp/editor/).
3. Klicke auf **Connect**.
4. Erlaube MIDI mit SysEx, falls der Browser danach fragt.
5. Öffne den Reiter **GRIND**.

Der Reiter zeigt sieben Generator-Einstellungen und die acht Spuren im Bereich **Keep steps**. Bei älterer kompatibler Firmware erscheinen nur die unterstützten Reiter. Die Demo dient zum Testen der Bedienung. Nutze den Simulator für hörbare Firmware-Wiedergabe.

### Einen Rhythmus erzeugen und variieren

Arbeite in einem Testprojekt oder sichere dein aktuelles Pattern zuerst. Ein neuer Stil kann das Raster und die Schritte aller acht Spuren ersetzen.

1. Stoppe die Wiedergabe am FM-1.
2. Öffne **GRIND**.
3. Wähle in **Style** den Stil **D-Beat**.
4. Klicke auf **Apply role kit**, falls du die acht passenden Instrumentklänge laden möchtest.
5. Bestätige die Änderung der Klänge im Dialog.
6. Starte die Wiedergabe am FM-1.
7. Wähle in **Apply at** zwischen **Next step** und **Pattern end**.
8. Klicke auf **New variation**.

Beim Live-Wechsel meldet der Editor zunächst `Waiting for the selected playback boundary.`, sofern die Änderung noch wartet. Nach der Übernahme zeigt er `Pattern applied.`. **Pattern end** bezieht sich auf die längste Spur. Beim Wechsel beginnen alle Spuren wieder bei Schritt 1.

| Ziel | Bedienung im Reiter GRIND |
| --- | --- |
| Dichte, Charakter und Timing ändern | **Aggression (%)**, **Density (%)**, **Chaos (%)**, **Humanize (%)** und **Mutation**. |
| Einzelnen Treffer oder eine Pause schützen | Unter **Track** die Spur wählen und die Schrittnummer anklicken. Erneutes Anklicken hebt den Schutz auf. |
| Mehrere Markierungen verwalten | **Keep track** schützt die Schritte der gewählten Spur. **Free track** löst diesen Schutz. **Free all tracks** hebt sämtliche Markierungen auf. |
| Letzte GRIND-Änderung umkehren | **Undo** wählen und bestätigen. **Redo** stellt das erzeugte Pattern wieder her. Spätere Schrittänderungen können dabei überschrieben werden. |

### Das Browser-Pattern auf dem FM-1 speichern

Im Browser speicherst du in das geladene Projekt. Wenn am Gerät noch kein Projekt geladen ist, wähle mit **SAVE** am FM-1 einen Slot und speichere dort zuerst.

1. Stoppe die Wiedergabe am FM-1.
2. Öffne **Patterns** im Editor.
3. Klicke auf **Save on FM-1**.
4. Prüfe, ob `Saved slot A on the FM-1.` erscheint. Der Buchstabe entspricht dem tatsächlich gespeicherten Slot.

Öffne anschließend **Projects**, wenn du zusätzlich eine Backup-Datei anlegen möchtest. Das Backup übernimmt den gespeicherten Stand des Slots. [GRIND-Reiter im Quellcode](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/web/editor/tab_grind.mjs) · [Speichern im Reiter Patterns](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/web/editor/tab_patterns.mjs)

Die Reiter **Sound**, **Presets**, **Projects**, **Patterns** und **Samples** ergänzen GRIND. **Projects** sichert und restauriert gespeicherte Slots. Stoppe den Transport vor einer Wiederherstellung oder einem Sample-Upload. Ein abgebrochener Sample-Upload kann den Zielslot leer hinterlassen. Behalte die Originaldatei. `VARY`, die LIVE-Regler, die Stilvorschau und die Ebenensperren bedienst du auf dem FM-1-Panel oder im Simulator. Der USB-Editor benötigt eine USB-Verbindung.

### Ein Projekt sichern und in einem anderen Slot prüfen

1. Stoppe die Wiedergabe und speichere das bearbeitete Projekt auf dem FM-1. Öffne im USB-Webeditor **Projects** und wähle beim Quellslot **Back up**. Prüfe, ob die Datei im Downloadordner liegt.
2. Wähle einen freien Zielslot. Falls du einen belegten Slot verwendest, sichere zuerst auch dessen Inhalt. Klicke am Zielslot auf **Restore here…** und öffne die zuvor heruntergeladene Projektdatei. Prüfe den Zielslot in der Rückfrage, bevor du das Überschreiben bestätigst.
3. Warte auf die Meldung **Restored into** mit dem Zielslot. Klicke auf **Read again**, lade den Zielslot auf dem FM-1 und spiele ein bekanntes Pattern ab.

**Mehrere Slots sichern.** **Back up all** sichert die gespeicherten Projekte einschließlich Variationen und Parametersperren sowie die 96 Benutzer-Presets mit ihren Effekten. Sichere eigene Samples separat als `.fsmp`. Bei **Restore all…** schreibt der Editor zuerst die Presets und danach die enthaltenen Projekte. Bricht der Vorgang ab, zeigt **Already restored** die bereits geschriebenen Einträge. Prüfe diese mit **Read again**, bevor du fortfährst.

### Eine Variation aus einem Slot oder einer Datei kopieren

1. Stoppe die Wiedergabe und lade auf dem FM-1 den Zielslot. Sichere ihn vorher, wenn du seinen Inhalt behalten willst.
2. Öffne **Patterns** und wähle **Copy variation…**.
3. Wähle unter **From** entweder **the FM-1** und den Quellslot oder **a file…** und **Choose a file…**. Bei einer Gesamtsicherung wähle zusätzlich das Quellprojekt.
4. Wähle die Quellvariation und unter **into … (loaded) VAR** die Zielvariation.
5. Wähle **Copy** und prüfe die Zuordnung in der Rückfrage. Das Kopieren ersetzt Schritte und Timing der Zielvariation. Wenn du die Option für die Locks einschaltest, ersetzt der Editor die Parametersperren des gesamten Zielslots. Das betrifft alle seine Variationen. Aus einer Datei werden keine `MOTION`-Ereignisse kopiert.
6. Bestätige die Kopie. Prüfe nach der Meldung `Copied …` die Zielvariation im Raster und höre sie ab. Stoppe anschließend die Wiedergabe und wähle **Save on FM-1**, wenn du das Ergebnis behalten willst. [Patterns](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/manual/14-web-editor.md#patterns)

### Ein Sample-Set sichern und zurückspielen

1. Verbinde den FM-1 mit dem Webeditor und öffne **Samples**. Die Karten **USR1**, **USR2** und **USR3** zeigen die gespeicherten Sets. Notiere den Namen und die Anzahl der Samples im Quellslot.
2. Wähle auf dessen Karte **Back up**. Prüfe, ob die Datei mit der Endung `.fsmp` im Downloadordner liegt.
3. Stoppe den FM-1. Wähle einen leeren Zielslot oder sichere zuerst dessen Inhalt. **Restore…** ersetzt das gesamte Set im Zielslot.
4. Klicke auf der Zielkarte auf **Restore…** und wähle die gesicherte Datei. Lass den FM-1 angeschlossen, bis der Editor die Wiederherstellung bestätigt.
5. Vergleiche Namen und Anzahl auf der Zielkarte. Höre mit **Play** ein bekanntes Sample ab.

### Einen Loop schneiden und auf den FM-1 schreiben

1. Sichere ein bereits belegtes Zielset. Stoppe die Wiedergabe und öffne auf dessen Karte **Chop a sound…**. Wähle eine WAV-Datei.
2. Wähle **Auto**, um Anschläge zu erkennen, **Beats** für ein musikalisches Raster oder **Equal** für gleich lange Teile. Bei **Auto** regelt **Sensitivity** die Zahl der Schnitte. Bei **Beats** gib das Tempo ein oder wähle **Detect**.
3. Klicke auf eine Schnittmarke und ziehe sie an die gewünschte Stelle. Ein Klick in die Wellenform ergänzt eine Marke, ein Doppelklick entfernt sie. Klicke zum Vorhören auf eine Slice-Nummer.
4. Prüfe vor dem Schreiben, wie viele Teile in den Zielslot passen. Ein Set fasst maximal 16 Samples. Bestätige die angezeigte Anzahl und prüfe anschließend die neuen Einträge auf der Zielkarte.

**Wann wird geschrieben?** Hinzufügen, Ersetzen und Schneiden übertragen das ganze Set sofort beim Bestätigen. Nach Verschieben, Umbenennen oder Löschen eines Samples wähle auf der Karte **Write USR1** (für Slot 1). **Undo changes** lädt den Gerätestand erneut. Bricht ein Upload ab, kann der Zielslot leer sein. Lass den Editor offen und schreibe das dort vorhandene Set erneut oder sichere es mit **Save as file**.

### Die Slices auf dem FM-1 spielen und aufnehmen

1. Wähle mit **ALGORITHM** die Spur. Öffne mit **EDIT** die Klangseiten und wähle auf der ersten **SOUND**-Seite mit **KNOB 1** das Modell `SAMPL`.
2. Wähle auf **SOUND 2/5** unter **SET** das Set mit deinen Slices, etwa `USR1`.
3. Stelle auf **SOUND 4/5** den Regler **PLAY** auf `CHOP`.
4. Aktiviere **KEYS** mit **SCL** am FM-1. Spiele die ersten weißen Tasten. Sie wählen Samples 1–16 in der Reihenfolge des Sets. Bei Slice 5 von 12 zeigt das Display `CHOP 5/12`. Schwarze Tasten und weiße Tasten ohne zugewiesenes Sample bleiben stumm.
5. Schalte die Spur für die Live-Aufnahme scharf und spiele die Slices wie im Aufnahmeabschnitt beschrieben ein. Spiel das Pattern anschließend ohne Aufnahme ab. Jeder aufgenommene Schritt spielt den Slice, den du eingespielt hast.

Mit `PLAY KEY` spielen die Tasten das unter **SLOT** gewählte Sample in verschiedenen Tonhöhen. In `CHOP` wählt MIDI-Note 60 (C4) Sample 1, Note 61 Sample 2 und so weiter. Die Auswahl wiederholt sich nach 16 Noten.

| Einstellung | Wirkung |
| --- | --- |
| **LOOP** → `LOOPI` auf **SOUND 5/5** | Wiederholt den Bereich von **START** bis **END** auch nach dem Loslassen der Taste. Der nächste Treffer der Spur, ein Choke oder **STOP** beendet die Schleife. |
| **LOOP** → `ONE` auf **SOUND 5/5** | Spielt von **START** bis **END**, unabhängig von der Notenlänge. |
| **CUT** → `ON` auf **SOUND 4/5** | Ein neuer Treffer beendet das noch klingende Sample derselben Spur. Bei `OFF` können sich Treffer überlagern. Der **CUT**-Regler auf der GRIND-Live-Seite stoppt die Wiedergabe. |

[Sample-Modell](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/manual/07-sounds.md#samples)

## MIDI und USB-Audio anschließen

Die Standard-Preview ohne Bluetooth bietet USB-MIDI und USB-Audio. Wähle am Gerät im Menü **USB AUDIO** die Kanalbelegung für deine DAW.

| Modus | Eingangskanäle in der DAW |
| --- | --- |
| `2 CH` | Stereo-Master auf 1-2 |
| `10 CH MIX` | Acht Mono-Spuren auf 1-8 und Stereo-Master auf 9-10 |
| `10 CH FX` | Acht Mono-Spuren auf 1-8 und Effektreturns für Chorus, Delay und Hall auf 9-10 |

Beim Wechsel zwischen 2 und 10 Kanälen meldet sich das Gerät nach dem Schließen des Menüs erneut an. Der Wechsel zwischen `10 CH MIX` und `10 CH FX` erfolgt direkt. Wähle die Eingänge in der DAW bei Bedarf neu. [USB-Audio](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/manual/11-usb-audio.md) · [MIDI](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/manual/10-midi.md)

### MIDI-Controller zuweisen und die Clock wählen

1. Verbinde einen MIDI-Controller, der CC-Daten sendet. Wähle am FM-1 die gewünschte Spur und öffne die Seite des Zielreglers.
2. Halte **GLO**, drücke die Taste **G4** und lasse **GLO** los. Bei `LEARN: TURN OR PRESS` drehe den Zielregler.
3. Bewege den Regler am Controller. Prüfe, ob die Anzeige dessen CC-Nummer und den gewünschten Spurparameter nennt. Beende mit **OCT+**. Bewege den Controller erneut und prüfe die Wertänderung.

| Aufgabe | Einstellung | Prüfung |
| --- | --- | --- |
| Der DAW folgen | Auf **GLOBAL 1/7** unter **CLK** die Quelle **USB** wählen. In der DAW die MIDI-Clock-Ausgabe zum FM-1 aktivieren. | Starte die DAW. Die Schritte laufen im DAW-Tempo, der Temporegler zeigt `CLK USB`. |
| Die DAW vom FM-1 takten | **CLK INT** und auf **SYSTEM 2/7** **SYNC USB** wählen. Die DAW auf externe MIDI-Clock stellen. | **PLAY** am FM-1 startet den Empfänger, **STOP** hält ihn an. |
| Eingehende Noten verteilen | **SYSTEM 2/7 → ROUT**: **NOTE** nutzt die Drum-Noten auf **CH**, **SEL** spielt die gewählte Spur tonal auf **CH**, **TRACK** ordnet Kanal 1–8 den Spuren 1–8 zu. | Sende bei **TRACK** Note 60 auf Kanal 2. Spur 2 erklingt in ihrer Grundstimmung. |
| Controllerdaten weiterleiten | Für einen Controller am TRS-Eingang **GLOBAL 3/7 → THRU ON** wählen und die USB-MIDI-Eingabe in der DAW beobachten. | Eine gespielte Note oder ein CC erscheint am USB-Ausgang des FM-1. |

### Reglergeschwindigkeit und Tastenbeleuchtung einstellen

Halte **HOME** etwa 0,7 Sekunden. Wähle mit **SELECT** die Zeile **ACCEL** und mit **KNOB 1** den Wert **OFF** für genau einen Werteschritt pro Rastung. Für Orientierung im Dunkeln wähle **BACKLIGHT → DIM LO**. Die unbetätigten Tasten leuchten schwach. Für die Displayfarben verwende **COLOR**. Halte **HOME** erneut, um das Menü zu schließen und zu speichern.

## Geräteeinstellungen zurücksetzen und Kalibrierung abbrechen

### Geräteeinstellungen zurücksetzen

Der Reset setzt unter anderem Farbthema, USB-Audio, Hintergrundbeleuchtung, Tastenbelegung sowie den GRIND-Stil und den Umschaltpunkt zurück. Gespeicherte Projekte, Presets und Samples bleiben erhalten.

1. Halte **HOME**, um das Gerätemenü zu öffnen.
2. Wähle mit **SELECT** die Zeile **RESET TO DEFAULT**.
3. Drücke **OCT+**. Die Zeile zeigt `SURE?`.
4. Drücke innerhalb von etwa 1,5 Sekunden erneut **OCT+**.

`DEFAULTS SET` bestätigt den Reset.

### Eine begonnene Kalibrierung abbrechen

1. Halte bei einer Tastenabfrage eine Taste etwa 1,5 Sekunden gedrückt. Bei einer Reglerabfrage drückst du kurz eine Taste.
2. Prüfe, ob `SETUP CANCELLED` erscheint.

Die bisherige Tasten- und Reglerbelegung bleibt erhalten. Nach etwa 30 Sekunden ohne Eingabe bricht die Kalibrierung ebenfalls ab. [Gerätemenü](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/firmware/src/ui_menu.c) · [Kalibrierung](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/firmware/src/ui_input.c)

## Song-Ausschnitte im Terminal untersuchen

Starte Python 3 im Repository-Verzeichnis. **list** zeigt die verfügbaren Titel. **compare** vergleicht viertaktige Fenster mit den 22 GRIND-Stilen. Beide Befehle lesen Daten lokal.

```sh
python3 tools/songs_pattern.py list assets/songs/rhythm_songs.json
python3 tools/songs_pattern.py compare assets/songs/rhythm_songs.json
```

Mit `show` und einem Titel aus der Liste zeigst du ein einzelnes Fenster an. `--bar` legt seinen Starttakt fest, `--shift` verschiebt die Treffer um ganze Sechzehntel. Für einen Transfer zeigt `python3 tools/songs_pattern.py send --help` die Optionen. `send` benötigt `mido` und `python-rtmidi`, einen gestoppten FM-1 und ein geladenes Projekt. Sichere das Zielpattern zuerst. `--save` speichert nach dem Schreiben. [Werkzeug und Referenzdaten](https://github.com/DaFum/grind-machine/tree/c9857e49d2270df0eb44052097bfafc39afa5286/assets/songs)

## Firmware installieren oder wiederherstellen

### Eine Preview-Installation vorbereiten

> **Achtung:** Preview .7 ist laut Projekt für physische FM-1-Geräte nicht qualifiziert. Ein Firmwarewechsel kann das Gerät vorübergehend unbenutzbar machen. Beginne bei unbekannter Gerätevariante oder ungeklärter Wiederherstellung im [Browser-Simulator](https://dafum.github.io/grind-machine/#play).

1. Ermittle die genaue Gerätevariante und die bisherige Firmware. Sichere Projekte, Presets, Samples und Einstellungen, soweit dein Ausgangssystem das unterstützt.
2. Lade den FM-1 auf. Verwende ein bekannt funktionierendes USB-Datenkabel.
3. Halte den offiziellen Updater, die passende Originalfirmware und die Wiederherstellungsanleitung bereit.
4. Vergleiche das Paket mit **Preview .7**, Quellstand `c9857e4`, und den veröffentlichten Prüfsummen. GRIND-Pakete tragen die Identität `FM-1_984`. Ein USB-Portname allein identifiziert das Firmwarepaket nicht.
5. Öffne den [GRIND-Webinstaller](https://dafum.github.io/grind-machine/webapp/installer/) in Chrome oder Edge. Die veröffentlichte Installer-Datei weist `GRIND MACHINE drum-1.0.1+c9857e4` aus. Folge der Anzeige und lass das Gerät während des Schreibens verbunden. Halte bei unerwarteten Meldungen an.

[Preview-Paket](https://github.com/DaFum/grind-machine/releases/tag/grind-v0.1.0-preview.7) · [Geräteinstallation](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/DEVICE_INSTALL.md) · [Kompatibilität](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/grind/compatibility.md)

### SAFE START und Rückkehr zur Originalfirmware

1. Schalte den FM-1 aus. Halte **SEQ** beim Einschalten gedrückt, bis `SAFE START` und `NO AUDIO - USB UPDATE READY` erscheinen.
2. Lass **SEQ** los. In diesem Zustand bleibt die USB-Updatefunktion verfügbar. Die Drum-Engine startet nicht. Schalte zum Verlassen aus.
3. Verwende für die reguläre Rückkehr **M-UPGRADE** und das passende offizielle FM-1-Paket. M-VAVE listete am 28. November 2026 **V16**. Prüfe vor dem Schreiben, welche Firmware für dein Gerät und dein Verfahren vorgesehen ist.
4. Wenn die normalen Updatewege scheitern, prüfe den Projektleitfaden für macOS im USB-Bootmodus. Dieser Weg erwartet ausdrücklich das geprüfte **V15**-Paket mit SHA-256-Präfix `db1642b2`. Er setzt ein erkanntes `WL80UBOOT1.00`, den dokumentierten Flash-Helfer und ein vollständiges Backup voraus.

Ein erkanntes USB-MIDI-Gerät, ein geladener Firmware-Container oder `Resuming OTA recovery` ist ein Zwischenstand. Bei `Timed out waiting for a MIDI device response` während der Übertragung ist der Schreibvorgang nicht als abgeschlossen bestätigt. Schließe andere MIDI-Programme, prüfe die Verbindung und folge dem dokumentierten Wiederherstellungsweg. Sichere das Fehlerprotokoll zur Diagnose.

[M-VAVE Downloads](https://www.m-vave.com/download) · [macOS-Wiederherstellung](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/docs/RECOVERY.md)

## Fehler beheben

| Symptom oder Meldung | Wahrscheinliche Ursache | Prüfung | Abhilfe |
| --- | --- | --- | --- |
| **ARP** zeigt nur `GRIDS` | GRIND wurde beim Build nicht aktiviert. | Prüfe den Build-Schalter und ob du die Seitenfolge vollständig durchlaufen hast. | Baue den Simulator mit `GRIND_NATIVE=1` und lade die neu gebaute Seite. |
| `KEEP DOES NOT FIT` | Geschützte Schritte passen nicht ins Zielraster. | Vergleiche die geschützten Positionen mit den neuen Spurlängen. | Löse nur die betroffenen Markierungen oder wähle einen passenden Stil. |
| `NOT APPLIED` | Schritte, Timing oder `KEEP` haben sich nach dem Vorbereiten geändert. | Prüfe die letzten Änderungen am Panel oder im Browser. | Löse die Erzeugung erneut aus. |
| Reiter **GRIND** fehlt | Die Firmware stellt die GRIND-Befehle für den Editor nicht bereit. | Vergleiche den Build des FM-1 mit der Preview im Installer. | Nutze für einen Gerätetest eine passende Firmware. Öffne für einen Test ohne Gerät die Editor-Demo. |
| Backup enthält das alte Pattern | **Projects** liest den gespeicherten Slot. | Prüfe die Speichermeldung unter **Patterns**. | Stoppe den FM-1, speichere mit **Save on FM-1** und erstelle das Backup erneut. |
| `STOP THE SONG` | Eine Song-Kette läuft. | Prüfe die Seite `SONG`. | Stoppe die Wiedergabe und wiederhole die Aktion. |
| `STOP FIRST` bei `KIT` | Der Transport läuft. | Prüfe die Transportanzeige. | Stoppe mit **PLAY** und bestätige `ROLE` erneut mit zwei Drehungen. |
| `AGAIN: ROLE KIT` erscheint erneut | Das Bestätigungsfenster ist abgelaufen. | Achte auf die Zeit zwischen den beiden Drehungen. | Drehe zweimal zügig nach rechts. |
| `NOTHING TO UNDO` | Der aktuelle Zustand enthält keine verfügbare GRIND-Änderung. | Prüfe, ob eine Erzeugung übernommen wurde. | Lade bei Bedarf die gespeicherte Projektfassung. Beachte die Rückfrage beim Verwerfen von Änderungen. |
| `SAVED (RAM)` | Dauerhafte Speicherung ist für diese Sitzung oder Gerätekonfiguration nicht verfügbar. | Prüfe im Browser die Speicherfreigabe. Lass beim FM-1 die Flash-Kompatibilität klären. | Sichere vorhandene Daten vor Neustart, soweit dein Setup das ermöglicht. Setze kein dauerhaftes Speichern voraus. |
| `PREVIEW WAITS` | Eine GRIND-Änderung wartet auf Übernahme. | Prüfe `WHEN` und den Transport. | Warte auf die Umschaltgrenze oder stoppe, um die wartende Änderung zu verwerfen. |
| `PENDING END` | Eine Spurmutation wartet auf das Pattern-Ende. | Prüfe, ob das längste Pattern noch läuft. | Warte bis zum Ende. |
| MIDI-Zeitüberschreitung beim Update | Der Updater erhielt keine erwartete MIDI-Antwort. | Prüfe Phase, Gerätemeldung und USB-Datenverbindung. | Folge dem Wiederherstellungsabschnitt und sichere das Protokoll. |
| Kein Ton | Transport gestoppt, Lautstärke niedrig oder Spuren stumm. | Prüfe Lauflicht, **MASTER**, Spurpegel und Solo/Mute. | Starte **PLAY**, prüfe den Audioausgang und hebe die betreffenden Stummschaltungen auf. |
| `LOAD FAILED: SLOT …` beim Einschalten | Der zuletzt gespeicherte Slot ist leer oder unlesbar. | Prüfe beim Einschalten mit gehaltenem **SEQ**, ob `SAFE START` erscheint. | Folge dem beschriebenen Wiederherstellungsweg. Lade eine vorhandene Projektsicherung erst nach regulärem Start. |

## Prüfstand

Diese Ausgabe dokumentiert [Preview .7](https://github.com/DaFum/grind-machine/releases/tag/grind-v0.1.0-preview.7), Commit `c9857e4`. Die Firmware zeigt `1.0.1`, der Preview-Tag identifiziert den GRIND-Build. Die Bedienabläufe wurden mit Firmware-Quellcode, Webeditor und den veröffentlichten GitHub-Pages-Dateien abgeglichen. Die Abbildungen verwenden das Panel-Foto des Projekts, LCD-Aufnahmen aus dem veröffentlichten WebAssembly-Build und schematische Bedienansichten.

Automatisierte Browser-Prüfungen in der Veröffentlichungsroutine ersetzen keine Prüfung auf einem physischen FM-1. Für diese Ausgabe wurde kein Firmwarepaket auf Hardware installiert. [Veröffentlichungsroutine](https://github.com/DaFum/grind-machine/blob/c9857e49d2270df0eb44052097bfafc39afa5286/.github/workflows/grind-preview.yml)

## Glossar

| Begriff | Bedeutung |
| --- | --- |
| Bank | Gruppe von 16 Schritten, die über die weißen Tasten erreichbar ist. |
| Cue | Vorgemerkter Wechsel zu einem anderen Projekt oder einer Variante. |
| GRIDS | Geerbter Rhythmusgenerator in derselben Seitenfamilie wie GRIND. |
| KEEP | Temporärer Schutz für Treffer und Pausen bei der GRIND-Erzeugung. |
| Parameter-Lock | Klang- oder Effektwert, den der Sequenzer für einen bestimmten Schritt verwendet. |
| Pattern | Abfolge der Schritte einer oder mehrerer Spuren. |
| Ratchet | Mehrere schnelle Treffer innerhalb eines Schritts. |
| Rolle | Instrumentaufgabe einer GRIND-Spur, etwa Kick oder Snare. |
| Transport | Start und Stopp des Sequenzers. |
| Variation | Alternative Schrittfolge eines Projekts. Der Regler `MUT` erzeugt einen neuen Entwurf. |
