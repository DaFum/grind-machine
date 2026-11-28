# BRUTAL EVOLUTION Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended where available) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Each BE package is a focused PR against current `main`; do not merge, release or flash without explicit authorization.

**Goal:** Sechs musikalisch wirksame Erweiterungen in GRIND MACHINE integrieren: LAYER LOCKS, TIMBRE DNA, BRUTAL PERC, SINE FOLDER, SOUND MORPH und MOVEMENT, mit deterministischer Mutation, sicherem nativen Commit und unveränderter Legacy-Audioausgabe bei deaktivierten Funktionen.

**Architecture:** Bestehende GRIND-Controller-, Stage-, Undo-, Step-, LFO-, FM- und INSERT-Pfade bleiben maßgeblich. Neue kleine Rezeptzustände werden getrennt von `gm_config` und von persistierten Track-Parametern gehalten; nur die zum Auftrag gehörenden Felder werden an der nativen Grenze veröffentlicht. Ein gemeinsamer Hit-Parameterpfad und eine versionierte Session-Schnittstelle vermeiden zusätzliche Pattern- oder Voice-Puffer.

**Tech Stack:** Portable C99, native gnu11-Amalgamation, Integer/Festkomma, JieLi PI32v2 mit gepinntem Compiler/SDK, bestehende Host-/WASM-/Native-CI, Web MIDI und ES-Module. Kein ARM-Port, keine neue Runtime-Abhängigkeit.

---

## 1. Referenzstand, Umfang und Verbindlichkeit

**Revision 2 · 28.11.2026.** Ersetzt den ersten Planentwurf. Die sieben BE00-Dateien sind eingesehen und rechnerisch abgeglichen; `main`, AGENTS.md, die gemergte Spezifikation und `runtime.c` wurden erneut per GitHub geprüft. Der aktuelle Main-Head ist unverändert `d63827e8cf8e8b9b41fa57ab3d0da079c5219e09`. BE00 statisch abgeschlossen, Gerätevalidierung weiterhin offen. Keine neuen Implementierungsfreigaben oder Alpha-Gates. Die folgenden Tasks sind Ausführungsanweisungen, keine bereits implementierten Änderungen.

- Repository: [DaFum/grind-machine](https://github.com/DaFum/grind-machine/tree/d63827e8cf8e8b9b41fa57ab3d0da079c5219e09), `main` zum Abruf: `d63827e8cf8e8b9b41fa57ab3d0da079c5219e09`. [PR #25](https://github.com/DaFum/grind-machine/pull/25) ist gemergt. Maßgeblich: [BRUTAL EVOLUTION Design](https://github.com/DaFum/grind-machine/blob/d63827e8cf8e8b9b41fa57ab3d0da079c5219e09/docs/superpowers/specs/2026-11-27-brutal-evolution-ui-memory-performance-design.md), [AGENTS.md](https://github.com/DaFum/grind-machine/blob/d63827e8cf8e8b9b41fa57ab3d0da079c5219e09/AGENTS.md), `CLAUDE.md` verweist auf AGENTS.md.
- DNA: [Design](https://github.com/DaFum/grind-machine/blob/d63827e8cf8e8b9b41fa57ab3d0da079c5219e09/docs/superpowers/specs/2026-11-27-grind-dna-engine-design.md) und [Plan D01-D07/T25-T38](https://github.com/DaFum/grind-machine/blob/d63827e8cf8e8b9b41fa57ab3d0da079c5219e09/docs/superpowers/plans/2026-11-27-grind-dna-engine-implementation.md) sind geplante, nicht bereits vorhandene Implementierungen. BE01 realisiert die gemeinsam benötigten nativen Invarianten; D05 muss diese wiederverwenden, nicht duplizieren. D01-D04, RIFF/FOLLOW, BREED und BRAIN sind keine Voraussetzung für BE02.
- Vorbild: [BRUTAL FM d2081886](https://github.com/hiroikezawa/brutal-fm/tree/d2081886152cc7fb716cdeb476ee56e9e4437cc0). Bei Portierung GPL-3.0-only und bestehende Copyright-Vermerke erhalten; `LICENSING.md` ergaenzen. Keine fremden Assets, kein Vieroperator-Port, HQ-Overampling, zweiter Renderer, Windows-Recovery oder BRUTAL SOUND LAB.
- Erste Hardware-Konfiguration: `GRIND_NATIVE=1 FELUCCA_BLE=0`. Bei `GRIND_NATIVE=0` keine neuen Symbole im Audiopfad, keine neuen UI-Seiten, keine neuen akzeptierten Insert-IDs und bitidentische alte Ausgabe. BLE-Fit nicht aus dem Nicht-BLE-Ergebnis ableiten. Native Transport-/Recovery-/OTA-Vertraege respektieren.
- Alte Style-IDs, Parameter-IDs, persistierte Felder, GMC1/GDN1/GDR1 und native Speicherformate nicht umnummerieren oder implizit erweitern. Keine neue Flash-Sektion, keine Verringerung der Voice-Cap oder des Pool-Floors. Keine Änderung der eingefrorenen Dateien laut AGENTS.md.
- Keine weiteren globalen Release-Gates, kein Aufschub der bestehenden Hardware-Alpha. Nachweise je PR sind Teil der vorhandenen Entwicklung und CI. Die neue Firmware wird nicht vor realen Messungen als hardware-echtzeitverifiziert bezeichnet.

### 1.1 Entscheidungen dieses Plans (keine bereits genehmigten Implementierungsdetails)

1. BE01 nutzt einen kompakten, separat verwalteten Rezeptzustand und auf einen Track begrenzte Mutations-Patches. Ein `native_revision`-Vergleich schuetzt die musikalische Basis; eine separate Session-/Exportrevision erfasst auch reine Live-Änderungen. Ein gemeinsamer Invalidierungspfad mit explizitem Änderungsumfang wird von späterem DNA D05 verwendet.
2. Erste RHY-Mutation unterstuetzt kompatible Grid-/Style-Regeln. Falls die aktive Struktur nicht ohne Rasterwechsel oder unzulaessige Änderung anderer Tracks dargestellt werden kann, `GM_LOCK_CONFLICT` oder `GM_UNSUPPORTED`, keine implizite Konvertierung. TIMBRE-only ist vom Rasterexport unabhängig; TIME-only verwendet die vorhandenen Zeitfelder, keine Neugenerierung des Rhythmus.
3. Timbralatch: beim ersten Step-Hit wird **der unskalierte** Timbre-Offset samt Ziel und logischer Position für den zugehörigen Ratchet gehalten. MOVE und MORPH bleiben live; das Grundrezept des Rolls bleibt stabil. Direkte MIDI-/Pad-/Grids-Hits und der sofortige REC-Monitorhit erhalten keinen sequenzierten DNA-Offset, wohl aber gültige MORPH-/BRUTAL-Werte. Der später abgespielte REC-Step hat seine eigene Kontur. REPEAT-Hits ohne eindeutige Step-Identitaet folgen ebenfalls dem direkten Pfad. Das ist eine bewusst begrenzte v1-Semantik.
4. Session-Import in v1 ist eine explizite, gestoppte Metadatenübernahme auf eine vom Host vollständig verifizierte passende Basis. Ein abweichendes Basisprojekt muss getrennt mit der bestehenden, ausdrücklich ausgeloesten Projektfunktion wiederhergestellt werden. Kein stilles Schreiben von Flash oder Projektparametern. Kein Versprechen eines automatischen vollständigen Restore, solange der End-to-End-Hostpfad nicht implementiert ist.
5. Der Rechenvorschlag von 1.600 B ist ein gemeinsames Entwicklungsziel, **kein gemessener Zuwachs und kein neues hartes Limit**. Bei Mehrbedarf priorisiert die nächste Phase zuerst das Zustandslayout; die bestehende harte RAM-Grenze bleibt unverändert.

## 2. BE00: Baseline und Ressourcenrechnung

### 2.1 Verifizierte statische Zielwerte

28.11.2026, Commit `d63827e8cf8e8b9b41fa57ab3d0da079c5219e09`, [Foundation CI: alle sieben Jobs erfolgreich](https://github.com/DaFum/grind-machine/actions/runs/46090369759), [Native-Job 140809614580](https://github.com/DaFum/grind-machine/actions/runs/46090369759/job/140809614580). Der Native-Log wurde für diesen Plan erneut gelesen: 426636 B Image, 95176/98304 B App-RAM, 323340/344064 B Pool, 909 RAMTEXT-Instruktionen ohne Calls, `PASS: resources`. Stack-Callpath main 4900 B, IRQ 1544 B.

Die folgenden Detailwerte wurden in Revision 2 anhand der bereitgestellten sieben Dateien überprüft. `felucca.map` liefert die tatsächlichen Abschnittsgrenzen; `layout.json` dokumentiert die Zielcompiler-Größenprobe, deren Messausdrücke in `layout.c` stehen. `resources.json` und `ci-resources.json` stimmen im vollständigen `checks`-Objekt überein (PASS statisch, BLOCKED Hardware). Sieben Artefakt-Hashes stimmen überein; der Hash der tatsächlich hochgeladenen `native-build.log` wurde neu berechnet und stimmt mit dem lokalen Report überein. Die Original-ELF-/Firmware-/Disassembly-Dateien sind nicht Teil der Uploads: deren Hashes sind Report-Vergleiche, keine erneute Binärprüfung. Die Angabe zum byteidentischen zusätzlichen Map-Linklauf stammt aus dem Messbericht; die Map wurde selbst geprüft, der Linklauf nicht wiederholt.

| Bereich | Belegt | Kapazität | Rest |
|---|---:|---:|---:|
| App-RAM | 95.176 B | 98.304 B | **3.128 B** |
| Pool | 323.340 B | 344.064 B | 20.724 B, davon 8.192 B Pflichtreserve |
| App-Flash | 426.636 B | 581.564 B | 154.928 B |
| RAMTEXT | 2.888 B | 24.576 B | 21.688 B |
| NOINIT | 15.576 B | 15.696 B | 120 B |
| Main-Stack, statischer Spitzenpfad | 4.900 B | 24.320 B | 19.420 B rechnerisch |
| IRQ-Stack, statischer Spitzenpfad | 1.544 B | 7.936 B | 6.392 B rechnerisch |

App-RAM: `.data` 300 B + Alignment 4 B + `.bss` 94.872 B. Flash: `.text` 423.448 B + RAMTEXT-Ladeabbild 2.888 B + `.data` 300 B. Pool über Floor: 12.532 B. Keine Addition dieser getrennten Regionen zum App-RAM. NOINIT ist keine frei verfügbare Scratch-Reserve. Die Stackwerte sind keine High-Water-Messungen; bestehende statische Stack-Mindestreserven 6.080/1.984 B ebenfalls einhalten.

| Symbol/Objekt | Zielgröße | Behandlung |
|---|---:|---|
| `gmrt` | 6.576 B | vorhandene Runtime erhalten |
| `gmrt.ctl` | 136 B | aktive cfg 24 B, pending/stage je 44 B; `gm_config` nicht aufblasen |
| `gmrt.undo` | 2.080 B | einzige native Pattern-Historie |
| `gmrt.out` | 2.144 B | Kandidat/Vorschau, nur bei exklusivem Besitz für Import-Scratch |
| `gmrt.u` plus Verwaltung | 2.216 B | Union 2.144, KEEP 64, next_rev 4, flags 2, alignment 2 |
| `trk[8]` | 20.128 B | je Track 2.516 B, nicht pauschal erweitern |
| `dvoice_t` | 444 B | zwei Modell- und zwei Layer-Stimmen je Track; +4 B je Voice würden 128 B kosten (vor Alignment) |
| `gm_pv`, `gm_v` | 208 + 36 B | Vorschau/Bedienung weiterverwenden |
| `mo`, `lk_slot`, `var_buf` | 6.040 / 12.832 / 7.168 B | keine neue Automationsbank, kein Flash-Umbau |
| `cv_px`, `ins`, `cho_buf` | 59.520 / 4.640 / 4.096 B (Pool) | keine zweite Canvas-/Effektinstanz |

Die Union-Ersparnis betraegt bereits 2.080 B, nicht 4.224 B; keinerlei Zusatzreserve. Die Größen für `mo`, `lk_slot`, `var_buf`, `cv_px`, `ins`, `cho_buf` sind aus dem bereitgestellten Messbericht übernommene ELF-Symbolwerte (die hochgeladene Map belegt die jeweiligen Regionen, nicht alle lokalen Symbolgrößen). Die `sizeof`-Werte für `gmrt`, Controller, Pattern, Track, Voice, Step, Preview und Bedienwerte sind hingegen unmittelbar in `layout.json` dokumentiert. Bestehende BLE-Einsparungen nicht erneut anrechnen. `gm_generate` Frame 2.616 B und `gm_rules_build` 896 B laut lokalem Messbericht: keine weitere volle lokale Pattern-Kopie.

Audio: `256 / 44100 = 5,805 ms` je Halbpuffer. Gewichtet-statische Schleifenkosten: `fm1_alnk0_irq` 11817/15188, `fm_render` 83/83, `ins_drive` 351/351, `lfo_track` 263/424. Keine dieser Zahlen ist eine CPU-Zyklus- oder Laufzeitmessung. Hardware `max_us`, `late`, IRQ-off-Dauer und Stack-High-Water: **OFFEN**, kein FM-1 im BE00-Messlauf verfügbar. Referenzdateien `firmware/src/audio.c`, `firmware/src/console.c`, `tools/stack_depth.py`, `tests/target_budget.py`.


### 2.1a Evidenzregister (sieben unveränderte Eingabedateien)

Alle SHA-256-Werte in der folgenden Tabelle wurden aus den hochgeladenen Dateien berechnet. Ablage der Rohdaten im Messarchiv des Auftraggebers; kein Firmware- oder SDK-Binärartefakt in den Quellcode-PR kopieren.

| Datei | Verwendung | SHA-256 |
|---|---|---|
| `layout.c` | Zielcompiler-`sizeof`-Probe | `ad33edc9df8d37991747aeea76cc81067735e74be395db29923be4836273dbd6` |
| `layout.json` | 30 Objektgrößen, 5 eigene Stackframes | `45b8bc2a8b7f7854f06c774800b85dbc6511ccbdec88bdabe323ab001f545bc5` |
| `native-build.log` | Buildzusammenfassung / vier Initialisierungswarnungen | `a1ee2e9e5369918ebadd002d9ae0276990f61a0c8423c8fd5a2decd027255b75` |
| `native-inputs.json` | Compiler-/SDK-Pinning | `58646e9af2f45fde384bb600c11d0bc279a2ca17c12e177dc6cbf46df443c70c` |
| `resources.json` | Lokale statische Ressourcenprüfung | `65b459775baf33be972c0dad4fb96781b5f2928a187da4ec97538f6f545c90fd` |
| `ci-resources.json` | Referenz-CI-Ressourcenprüfung | `6646debb4b95b1ec9fb5b30a4f627229118d60ad3b130501b29adb167636c4af` |
| `felucca.map` | Linkerregionen und Abschnittsadressen | `36c8facd3d743a9730d6e0a51656cc6e892e7bc2ef5f0c11ad47f901f4b5166e` |

**Nachrechenbare Abschnittsbilanz:** `.data` 0x12c, vier Byte Lücke vor `.bss`, `.bss` 0x17298; RAM-Ende 0x1c1f3c8, Regionende 0x1c20000. `.text` 0x67618, `.ram_text` 0xb48, `.pool` 0x4ef0c, `.noinit` 0x3cd8. XIP-Ladeabbild = `.text` + `.ram_text` + `.data`. Die Stackpfade stammen aus `resources.json`, die einzelnen Framegrößen aus `layout.json`. Die Linker-Map allein ist keine Stack-High-Water- oder Laufzeitmessung.

**Reproduzierbarer Abgleich für späteren Reviewer** (im Verzeichnis der sieben Dateien ausführen, read-only):

```python
import hashlib, json, pathlib, re
p = pathlib.Path('.')
a = json.loads((p/'resources.json').read_text())
b = json.loads((p/'ci-resources.json').read_text())
assert a['checks'] == b['checks']
assert a['static_status'] == b['static_status'] == 'PASS'
assert a['hardware_status'] == b['hardware_status'] == 'BLOCKED'
for k in ('app_image','app_elf','target_listing','loader',
          'text.bin','data.bin','ramtext.bin'):
    assert a['artifacts'][k]['sha256'] == b['artifacts'][k]['sha256'], k
assert hashlib.sha256((p/'native-build.log').read_bytes()).hexdigest() == \
       a['artifacts']['native_build_log']['sha256']
text = (p/'felucca.map').read_text()
regions = {k:(int(x,16),int(y,16)) for k,x,y in re.findall(
    r'^(XIP|RAMTEXT|RAM|POOL|NOINIT)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)',text,re.M)}
sections = {k:(int(x,16),int(y,16)) for k,x,y in re.findall(
    r'^(\.text|\.data|\.bss|\.ram_text|\.pool|\.noinit)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)',text,re.M)}
ram = sections['.bss'][0]+sections['.bss'][1]-regions['RAM'][0]
flash = sum(sections[k][1] for k in ('.text','.ram_text','.data'))
assert (ram,flash,sections['.pool'][1],sections['.ram_text'][1],sections['.noinit'][1]) == \
       (95176,426636,323340,2888,15576)
assert regions['RAM'][1]-ram == 3128
assert regions['POOL'][1]-sections['.pool'][1]-8192 == 12532
s=json.loads((p/'layout.json').read_text())['target_sizeof_bytes']
assert s['gmrt']==6576 and s['gm_config']==24 and s['gm_request']==44
assert s['gmrt.u.base']+s['gmrt.u.n']-s['gmrt.u']==2080
assert s['trk']==8*s['track_t']==20128 and s['dvoice_t']==444
print('BE00 static evidence: PASS; hardware: BLOCKED')
```

Die vier bestehenden `-Wmissing-braces`-Warnungen sind kein beobachteter Funktionsfehler; nicht als Nebenaufgabe im BE01-PR ändern. Der Build trägt weiterhin `UNTESTED`; statisches PASS bedeutet ausdrücklich nicht Hardwarefreigabe. SDK-Tag `AC79NN_SDK_V1.2.1_2023-12-13`, gepinnter Commit `d179b4484759423312073f5fbb232501aa491047`, Zielcompiler 4.0.1; die Eingabedatei dokumentiert die Prüfsummen, ohne dass die Compiler-/SDK-Archive selbst erneut gehasht wurden.

### 2.2 Vorgeschlagene zusätzliche App-RAM-Ziele (netto je Paket)

| Paket | Ziel netto | Kumuliert | Rest vom Ausgangsstand |
|---|---:|---:|---:|
| BE00 | 0 B | 0 B | 3.128 B |
| BE01 | 448 B | 448 B | 2.680 B |
| BE02 | 192 B | 640 B | 2.488 B |
| BE03 | 128 B | 768 B | 2.360 B |
| BE04 | 64 B | 832 B | 2.296 B |
| BE05 | 640 B | 1.472 B | 1.656 B |
| BE06 | 64 B | 1.536 B | 1.592 B |
| Gemeinsames Alignment-/Integrationsbudget | 64 B | **1.600 B** | **1.528 B** |

Die App-RAM-Zielbelegung am Ende wäre 96.776/98.304 B (98,45 %), **nur ein Planwert**. Das Restbudget gehört auch zu künftiger DNA-Integration; keine zweite Zusage. Alle vier vollständigen 560-B-Snapshots der Spezifikationsillustration würden 2.240 B kosten, fünf 2.800 B; verbleibend 888/328 B. Stattdessen gezielte Patches mit getrenntem Besitz. Keine Verwendung historischer BLE-Ersparnisse. SINE soll keinen neuen Pool-Puffer brauchen; jeder Netto-Pool-Zuwachs ausweisen und Floor erhalten. Keine pauschalen Flash-/CPU-Zahlen für noch nicht geschriebenen Code. Zielcompiler-Map und `sizeof`-Probe je PR aktualisieren; bei >98.304 B Arbeit nach AGENTS.md stoppen und gemessenen Wert melden.


**Aufschlüsselung der Planwerte, keine Zielcompiler-Messwerte:**

| Paket | Feldgruppen und maximaler Planansatz | Summe |
|---|---|---:|
| BE01 | aktive Rezepte 8×20 = 160; Undo-Rezepte 8×20 = 160; Pending/Stage je 36 = 72; vorherige `gm_config` 24; fünf u32 Verwaltungswerte 20; vier u8 Scope-/Statusflags 4; vier i16 UI-Proxies 8 | 448 B |
| BE02 | Ratchet-Latch 8×4 = 32; bis 8 B Hit-Context-Verwaltung; bis 8 B UI-Proxies; bis 144 B übrige Resolver-/Preview-/Ausrichtungsreserve | 192 B |
| BE03 | Mode/BRUT bereits in Rezept enthalten; bis 8 B UI-Proxies/Status; bis 120 B Reserve für nachgewiesen notwendigen Zusatzstate (keine pauschale Voice-Vergrößerung) | 128 B |
| BE04 | 8×3 B SINE-Fade-Ausgangsparameter = 24; bis 8 B Validität/Alignment; 32 B Reserve | 64 B |
| BE05 | 8×2×20 B Endpunkte = 320; 8×2 B POS/ON = 16; bis 64 B Import-/Export-Owner, Token, CRC, Offset/Timeout; bis 8 B UI-Proxy; 232 B Reserve für Protokollverwaltung/Alignment (kein zweiter 576-B-Puffer) | 640 B |
| BE06 | globales MOVE und UI-Proxy bis 4 B; 60 B Reserve für Synchronisation/Integration; keine zusätzlichen LFO-Phasen | 64 B |
| gemeinsam | Netto-Alignment und sonstige integrierte statische Änderungen | 64 B |
| Gesamt | Zielwert; nicht jede Reserve muss ausgeschöpft werden | 1.600 B |

BE01-Vorschlag für die fünf u32 Verwaltungswerte: `context_epoch`, `undo_identity`, `undo_context`, `undo_base_revision`, `session_revision`. Vorhandene Controller-Revisionen und `gmrt.undo_ok/undo_was_undo` weiterverwenden. Die vier u8 sind z. B. Undo-Trackmaske, Undo-Layer-/Write-Set, History-Kind und physischer Worker-/Scratch-Owner. Falls zusätzliche Live-Versionsfelder nötig sind, sie in der Phasenreserve explizit bilanzieren, nicht still neue Arrays hinzufügen. Pending-/Stage-Gültigkeit aus existierenden Controllerzuständen plus exakt übereinstimmender Request-ID ableiten. Compile-time Größenassertionen für vorgeschlagene POD-Strukturen; den **Netto**-Zuwachs der gesamten Firmware einschließlich Alignment, zusätzlicher Proxies und initialisierter `.data` per Map beweisen. Die Zahlen sind Ziele, nicht künstliche harte Gates. Physische Besitz- und Synchronisationsanforderungen haben Vorrang vor einer geschätzten Strukturgröße.

### 2.3 Tasks BE00 (Dokumentation / Messartefakte, kein Firmware-Patch)

**BE00.1** (`docs/superpowers/plans/2026-11-28-brutal-evolution-implementation.md`, optional im später autorisierten Doku-PR `docs/superpowers/specs/2026-11-28-be00-resources.md`):
- [x] Main-Head, gemergte Spec und Native-CI-Kernwerte erneut abgleichen.
- [x] Die sieben vorliegenden Rohdateien geprüft: Map/Abschnitte, Größenprobe, Eingaben, identische Checks, sieben übereinstimmende Artefakt-Hashes, tatsächlichen lokalen Buildlog-Hash nachgerechnet; Evidenzregister in §2.1a.
- [ ] Original-ELF/App/Loader und erneuter Linklauf nur falls künftig benötigt mit den hinterlegten Hashes nachweisen; die Binärdateien wurden nicht hochgeladen. Keine Pflicht zum Wiederholen des unveränderten bereits belegten statischen Builds allein für den Beginn von BE01.
- [ ] Vor Start jedes Implementierungs-PRs `git rev-parse HEAD`, `git status --short`, `git diff --stat origin/main...HEAD` dokumentieren; bei neuer Basis gleiche Buildkonfiguration wiederholen. Kein Reset fremder Änderungen.

**Reproduktion laut bereitgestellter Messung, gepinnte Eingaben vorher mit `tools/grind/native_inputs.py` prüfen:**

```bash
set -o pipefail
mkdir -p build/be00
GRIND_NATIVE=1 FELUCCA_BLE=0 JIELI_DOCKER=0 DRUM_SITE=0 DRUM_PACKAGE=1 \
  ./build.sh 2>&1 | tee build/be00/native-build.log
python3 tools/grind/resource_report.py --build-dir build \
  --build-log build/be00/native-build.log > build/be00/resources.json
"$JIELI_TOOLCHAIN/pi32v2/bin/ld" -T firmware/app.ld build/crt0.o \
  build/fm1_vec.o build/fm1_isr.o build/felucca.o \
  -Map build/be00/felucca.map -o build/be00/map-check.elf
cmp build/felucca.elf build/be00/map-check.elf
sha256sum build/felucca.bin build/felucca.elf build/loader/ota.bin
```

Gemeldeter App-Image-Hash: `ec910287ff66784eb15922c1bb4351365b5050139628e29c0a30c03432dec107`. Umgebung: Linux x86-64, Python 3.12, Pillow 13.0.0, JieLi pi32v2 clang 4.0.1, SDK-Commit `d179b4484759423312073f5fbb232501aa491047`. `cmp` muss 0 liefern. CI-Native-Lane `bash tools/grind/run_ci.sh native` mit den in CI geforderten Eingaben `GRIND_JIELI_ARCHIVE`/`AC79_SDK`; fehlende Eingaben als BLOCKED ausweisen, nicht Host-Build als Ersatz ausgeben.

**BE00.2 / Hardware-Nachweis (nicht als Voraussetzung für Codebeginn):**
- [ ] Wenn ein autorisiert verfügbares Geraet existiert: bestehende Diagnose und reproduzierbares Lastpattern dokumentieren. Gleicher Build, Sample-Rate, Tempo, Clock-Modus, Kit/Track-Zustand, Insert-Mix und Host-MIDI-Trace; zunaechst unveränderter Baseline-Build messen. Dichte Mehrspurpatterns, Ratchets, lange FM-Ausklänge, hohe Tune/Feedback, Inserts, gleichzeitige UI/MIDI. Anfangs-/Endstand von `late`, gerenderte Halbpuffer und `max_us` inklusive Messzeitfenster notieren; `max_us` zieht verschachtelte TIMER5-Zeit ab. High-Water beider Stacks und maximale IRQ-off-Dauer gesondert beobachten, ohne Recovery/Linker/HAL umzubauen. Keine unautorisierten Hardware-Schreibvorgaenge. Bei fehlendem Geraet: OFFEN, nicht bestanden.
- [ ] Spaetere Featurevergleiche mit identischem Szenario, aktiver Featurelast und deaktivierten Features. Statische Indizes gesondert halten; keine Umrechnung in erfundene Mikrosekunden.

## 3. PR-Aufteilung, Dateikarte und gemeinsame Testregeln

| PR | Branchvorschlag | Tasks | Abhängigkeit / lieferbares Ergebnis |
|---|---|---|---|
| BE00 | `docs/be00-resource-baseline` (falls benötigt) | BE00.1-.2 | statische Basis vorhanden; Hardware offen |
| BE01 | `feat/be01-layer-locks` | T01-T04 | BE00; nativer Schutz/Undo + RHY/TIME-Locks |
| BE02 | `feat/be02-timbre-dna` | T05-T07 | BE01; sechs Konturen, echte FM-Hit-Integration |
| BE03 | `feat/be03-brutal-perc` | T08-T09 | BE02; alternative FM-Modi und Bedienelemente |
| BE04 | `feat/be04-sine-folder` | T10-T11 | BE00; kann unabhängig vorbereitet werden |
| BE05 | `feat/be05-sound-morph` | T12-T14 | BE02/BE03; A/B, Capture, hitweiser Morph, Session-Transport |
| BE06 | `feat/be06-movement` | T15-T17 | BE02, BE05 für gemeinsame Integration; BE04 nur für SINE-Ziele; End-to-End |

PRs stets auf aktuelles `main` (kein gestapelter Merge auf unbeabsichtigten Feature-Branch). BE04 parallel nur wenn Dateikonflikte und Ressourcenbilanz beim späteren Integrieren erneut geprüft werden. BE05 setzt keine SINE-Implementierung voraus. BE06 darf sein Allowlist-Modul frueher entwickeln; nicht zwei konkurrierende Parameterresolver implementieren. Das DNA-T32/T33/T36-Paket muss aktuelle BE-Hooks und reservierte Wire-IDs übernehmen. Bei gemeinsamem Bedarf denselben vorhandenen Controller und dasselbe Undo nutzen.

**Dateikarte (neue Dateinamen sind Planentscheidungen):**

| Einheit | Datei(en) | Verantwortung |
|---|---|---|
| Portable BE-Typen/Mathe | neu `firmware/src/grind/be.h`, `be_math.c` | reine C99-Typen, Key/Contour/Morph, keine Target-Header |
| Layer-Kandidat | neu `firmware/src/grind/be_layers.c`, bestehend `grind/core.c`, `rules.c`, `seq_adapter.c`, `runtime.c`, `controller.h/.c`, `amalgam.c` | portable Transformation, native Erhaltung, scoped publication |
| Native BE-Besitz | neu `firmware/src/grind/be_runtime.c`, `core.h`, `felucca.c` | State/Token/Undo/Context, nur GRIND_NATIVE; nicht global in Legacy-Build |
| Klangauflösung | neu `firmware/src/grind/be_sound.c`, `dm_fm.c`, `dm_dsp.c`, `drum_core.c`, `seq.c`, `motion.c`, `lfo.c` | Hit-Kontext, nicht persistente Basis, vorhandene Automation |
| Insert | `core.h`, `insert.c`, `params.c`, `ui_draw.c`, `project.c`, `presets.c` (Verbraucher-Audit), `editor.c`, `editor_pattern.c`, `editor_backup.c`, `web/editor/*` soweit ID-Consumer | append-only SINE, Wet/Type-Transitions und Kompatibilitaet |
| Panel | `grind/ui.c`, `pages.c`, `ui.c`, `ui_draw.c`, `ui_input.c`, `tests/ui_host.h`, `tests/sim_host.h` | Scope-/Descriptor-Routing, UI-Status, echte Bedienung |
| Edit-/Reset-Audit | `seq.c`, `motion.c`, `midi_learn.c`, `project.c`, `song.c`, `cue.c`, `ui_input.c`, `ui_tools.c`, `editor.c`, `editor_pattern.c`, `editor_backup.c`, `presets.c`, `tests/grind/test_edit_sites.py` | alle tatsächlichen Schreibpfade identifizieren; nur notwendige Dateien ändern |
| Session | neu `grind/be_session.c`, `editor_proto.h`, `editor.c`, `web/editor/proto.mjs`, `session.mjs`, `tab_grind.mjs`, neu `be_session.mjs`, `mock_device.mjs` | versionierte Meta-/Basis-Chunks, Token, validiertes Apply, kein Flash |
| Nachweise | `tests/grind/test_be_*.c`, `tests/run_grind_tests.sh`, `tests/editor_test.c`, `web/editor/test_be_session.mjs`, `LICENSING.md`, `docs/manual/`, `README.md` | Red/Green, Regressionen, Benutzervertraege |

Vor jeder Aufgabe den aktuellen Dateibestand und Include-Order prüfen: `felucca.c` inkludiert `dmodels.c`/`dm_fm.c` vor `params.c`, `lfo.c`, `drum_core.c`, `seq.c`; `seq.c` inkludiert `motion.c`, `song.c`, `cue.c`, MIDI-Code; dann `grind/amalgam.c`, Adapter, Runtime und UI. Statische Vorwaertsdeklarationen für spätere Helpers in `core.h` unter `#if GRIND_NATIVE`; neue native Includes außerhalb des `FELUCCA_OTA`-Blocks. Native Testharnesses müssen dieselbe tatsächliche Logik inkludieren, keine no-op-Stubs für die Funktion, die der Test beweisen soll. UI-Hooks immer in `tests/ui_host.h` und `tests/sim_host.h` bereitstellen. Alle neuen portablen `.c` in `grind/amalgam.c` und benötigten Host-/WASM-Buildlisten registrieren.

### 3.1 Test-/Commit-Schleife je Task

- [ ] Eine unabhängig berechnete Fixture mit echten Vorher-/Nachher-Daten erstellen, in `tests/grind/test_<suite>.c` oder explizitem Node-Test. Keine Selbstvergleiche. Zuerst Suite in `case` **und** `all` registrieren (bei nativen Suites `run_native`; portable mit `run_test`/benötigten C-Dateien; bei UI-Harness passende Includes).
- [ ] `sh tests/run_grind_tests.sh <suite>` vor Implementierung: erwarteter funktionaler RED (fehlende API darf nur beim ersten Testgeruest RED sein; danach gezielte Assertion-Failure). Exitcode und konkrete Assertion dokumentieren. Falls bereits gruen: Test schaerfen oder bestehendes Verhalten belegen, nicht unnnötig patchen.
- [ ] Minimalen Patch implementieren, gezielte Suite erneut ausführen (Exit 0 und alle Assertions). Zusatzregressionen laut Task. `git diff --check`, Scope und Testmutanten prüfen; erst danach lokaler Commit (nur bei autorisierter Implementierung).

Beispiel Registrierung ab BE01:

```sh
# case in tests/run_grind_tests.sh
be_layers) run_test be_layers "$CORE/be_layers.c" "$CORE/be_math.c" \
    "$CORE/grid.c" "$CORE/rules.c" "$CORE/core.c" "$CORE/locks.c" \
    "$CORE/rng.c" "$CORE/serialize.c" ;;
be_runtime) run_native be_runtime ;;
# dieselben run_test/run_native-Aufrufe im all-Zweig
```

Zum Abschluss jedes PRs die jeweils vorhandenen Foundation-Lanes nutzen, keine neue CI-Policy:

```bash
python3 tools/build.py --gen-only
sh tests/run_grind_tests.sh all
GRIND_SAN_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all' sh tests/run_grind_tests.sh all
python3 -m unittest discover -s tests/grind -p 'test_*.py' -v
python3 tools/check_untouched.py
node web/editor/test_editor.mjs
# Linux CI: vorhandene Lane mit jeweiliger Umgebung
bash tools/grind/run_ci.sh host-core
bash tools/grind/run_ci.sh host-upstream
bash tools/grind/run_ci.sh performance
# gepinnte Eingaben bzw. Docker/Emscripten/Chromium vorausgesetzt:
bash tools/grind/run_ci.sh native
bash tools/grind/run_ci.sh wasm
bash tools/grind/run_ci.sh license
```

Nur tatsächlich verfügbare Lanes ausführen und BLOCKED/Skipped unterscheiden. Host-Test `run_native` ist **native Firmware-Quellcode auf Host**, kein PI32v2-Build. Die portable WASM-Suite kann mit `GRIND_WASM=1 sh tests/run_grind_tests.sh <suite>` laufen; native Host-Fixures werden dort erwartungsgemaess ausgelassen und sind kein WASM-Nachweis. Fuer `tests/boot_test.c`, `tests/ui_test.c`, `tests/editor_pattern_test.c` beide GRIND-Varianten gemaess AGENTS.md kompilieren und starten; vorher die Verzeichnisse aus `tests/run_drum_tests.sh` anlegen. Legacy-Goldens nicht neu generieren; CRLF unter Windows beachten. Zusaetzliche neue Testgoldens sind unabhängige Fixtures, keine aktualisierten alten Sollwerte.

## 4. BE01 - LAYER LOCKS und gemeinsame Transaktion (T01-T04)

### T01 - Kompakte Zustandsverträge und reine Layer-Transformation

**Files:** create `grind/be.h`, `grind/be_math.c`, `grind/be_layers.c`, `tests/grind/test_be_layers.c`; modify `grind/amalgam.c`, `tests/run_grind_tests.sh`. Pfade unter `firmware/src/`, außer Testpfaden.

**Geplante Typen und API:** keine bestehenden Strukturen umdefinieren; v1 `be_recipe` 20 B, `be_patch` maximal als kleines Payload (Größe im Zielbuild messen). Keys sind wirksame 32-bit-Schluessel, nicht die UI-Request-ID. Kein serialisiertes C-Padding.

```c
/* be.h -- proposed; portable, SPDX GPL-3.0-only */
#include "grind.h"
enum { BE_RHY = 1u, BE_TIME = 2u, BE_TMBR = 4u, BE_ALL = 7u };
enum { BE_ASCEND, BE_DESCEND, BE_ALTERNATE, BE_CHAOTIC, BE_MOTIF, BE_PULSE, BE_CURVE_N };
enum { BE_INDEX, BE_MDEC, BE_FBK, BE_TARGET_N };
enum { BE_OFF, BE_METAL, BE_GROWL, BE_CHAOS, BE_NOISE, BE_MODE_N };
typedef struct {
    uint32_t key[3]; /* RHY, TIME, TMBR in this order */
    uint8_t curve, amount, span, target;
    uint8_t mode, brutal, locks, flags;
} be_recipe;
typedef char be_recipe_is_20[sizeof(be_recipe) == 20 ? 1 : -1];
typedef struct {
    uint32_t request, base_revision, context;
    be_recipe next;
    uint8_t track, layers, changed, reserved;
} be_patch;
/* No pointer to transient UI storage. */
uint32_t be_key_next(uint32_t key);
gm_result be_merge_cell(gm_cell base, gm_cell candidate, unsigned layers,
                        int protected_cell, gm_cell *out);
int8_t be_time_micro(const gm_config *cfg, uint32_t key, unsigned track,
                    unsigned step, uint8_t swing);
```

`be_key_next` uses `gm_mix32(key ^ 0x9e3779b9u)`; default keys derived by `gm_mix32(seed ^ track/layer-specific domain)` in a versioned mapping; never use common mutation counter as all three effective keys. Reserve distinct BE domains from 15 onwards (DNA design reserved 8-14); recheck merged code/domain table before registration. Key advancement candidate only, publish once on actual `GM_OK` changed state. Avoid all-zero sentinel dependence; all key values including 0 valid. `flags` v1 defined bit0 timbre-active, remaining bits zero; all unknown mask bits rejected. Init `locks=0, amount=0, span=16, target=BE_INDEX, curve=BE_ASCEND, mode=BE_OFF, brutal=0` (neutral).

**Red:** independent fixture `base={GM_ON|GM_ACC|GM_TIE|(2<<GM_RAT_SHIFT),7,12,-25}` and candidate `{GM_ON|GM_SLIDE,3,-5,9}`; protected result exact base; RHY-only flags/cond/note candidate and micro -25; TIME-only all base fields except micro 9; zero layers equal base. Out-of-range flags/condition/note/micro, mask outside BE_ALL or null destination => `GM_INVALID`, destination unchanged. `BE_TMBR` alone does not touch cell bytes. Test protected OFF with nonzero dormant attributes (native preservation later). Deterministic keys for different tracks/layers, reject shared-counter mutation.

```c
/* Example portable fixture, register in test_be_layers.c */
gm_cell a = { GM_ON | GM_ACC | GM_TIE | (2u << GM_RAT_SHIFT), 7, 12, -25 };
gm_cell b = { GM_ON | GM_SLIDE, 3, -5, 9 }, out;
assert(be_merge_cell(a, b, BE_RHY, 0, &out) == GM_OK);
assert(out.flags == b.flags && out.cond == 3 && out.note == -5 && out.micro == -25);
assert(be_merge_cell(a, b, BE_TIME, 0, &out) == GM_OK);
assert(out.flags == a.flags && out.cond == 7 && out.note == 12 && out.micro == 9);
assert(be_merge_cell(a, b, BE_ALL, 1, &out) == GM_OK);
assert(!memcmp(&a, &out, sizeof a));
```

- [ ] Run `sh tests/run_grind_tests.sh be_layers`, expect RED on the selective fields.
- [ ] Implement `be_merge_cell` using `next=base`, only replace rhythm fields when RHY and not protected, only replace micro when TIME and not protected. Validate both input cells with existing `valid_cell` bounds (expose or factor common pure check without altering old behavior), validate mask before writing `*out`. For TIME-only compute micro from the existing span formula in `core.c:113-123` with independent TIME key; use copy of cfg with effective TIME mutation and the same `gm_random` coordinates. Step 0 nonnegative; other steps symmetric; never change rhythm because a generator did not choose the hit. On inactive untouched cells preserve micro; when RHY creates a hit and TIME free, compute its timing from TIME key, when TIME locked retain native micro.
- [ ] Add RHY candidate wrapper using existing `gm_generate` into `gmrt.out` later at T02, not another complete `gm_pattern`. Feed fixed selected-track key into local config; only use generated rhythm fields of selected track, preserve all other track fields. For combined portable validation reconstruct final playable cells from base + candidate in place, copy timing from base, only selected TIME swing if explicitly enabled; v1 TIME mutation chooses micro, existing swing stays unchanged. Keep protected PLOCK/MOTION and KEEP as temporary whole-cell/timing restrictions. Compare selected-grid length/div/source with rule output; reject unsupported grids rather than editing unrelated timing. Use `gm_validate` on the merged playable pattern and check affected rule anchor/forbidden/paired/ratchet masks where an actual change would violate them; don't silently make forbidden new hits or break required grouping. No extra changes to fix unrelated existing manual cells. If original grid is non-representable, RHY refused, TIME-only supported only when native timing/voice limits can be verified; TIMBRE-only independent. Tests: another track with a required paired hit that cannot match -> conflict, protected OFF remains OFF, changed rhythm within one supported track, no-change not advancing key.
- [ ] Green `be_layers`, `locks`, `properties`, `golden`; `GRIND_WASM=1 sh tests/run_grind_tests.sh be_layers` in configured WASM environment. Commit `feat(grind): define independent layer mutation contracts`.

### T02 - Scoped native publication, dormant data and request ownership

**Files:** create `grind/be_runtime.c`, `tests/grind/test_be_runtime.c`; modify `grind/runtime.c`, `controller.h/.c`, `seq_adapter.c`, `core.h`, `felucca.c`, native harness inclusion (`tests/drum_host.h` etc), runner. Existing `gm_runtime_submit/poll/commit` and cue are the authority.

**State layout proposal:** active `be_recipe[8]` = 160 B; undo recipe storage for 8 tracks = 160 B (legacy full mutation can change all identities); pending and stage payload each `be_patch` = 36 B expected; remaining 56 B for context/snapshot identity, previous config (existing `gm_config` 24 B), masks/alignment within BE01's 448-B target. Count actual target `sizeof`, not only arithmetic. `be_pending` is latest, `be_stage` immutable from WRITING through acknowledgement. Full A/B arrays are active-only from BE05, not copied in every request. Metadata captured at commit into undo, not at submit. `gmrt.undo` remains sole native undo. Add small scope/validity flags, not full pattern. If a legacy commit replaces all generator identities, preserve old metadata of all affected tracks, not newer independent live performance fields. No change of old legacy pattern generation path.

Proposed native API (defined once in `be_runtime.c`, prototypes in `core.h` as necessary for textual include):

```c
static gm_result be_submit(uint8_t track, uint8_t layers); /* main; fixed basis */
static gm_result be_stage_prepare(const gm_request *r);   /* main, WRITING only */
static void be_commit_isr(uint32_t request);               /* already protected */
static void be_context_isr(void);                         /* context, no nested guard */
static void be_edit_isr(unsigned track, unsigned change_mask); /* real edit only */
```

The existing controller receives append-only request kind `GM_REQUEST_BE_LAYER` (no renumber); fixed `base_revision`, `follow_live=0` for the explicit scoped request; legacy knob behavior unchanged. Capture selected track/locks/config/version/request once. Inside guarded transition: store/transfer matching patch by request ID; when superseded or cancelled, invalidated owner cannot publish. No second queue. A logical cancel can make the controller FREE before the main-loop writer has returned; its physical scratch ownership is released only on actual worker exit. ISR cancellation never permits reuse by another writer, preview or import while the worker still owns the bytes. STOP drops pending/ready, legacy slot/variation wins, SONG/BAR refused, `END=GM_BOUNDARY_PATTERN` while playing, stopped commit immediately after current guards including queued PLAY/restore considerations. `gm_ctl_finish` and consume verify request/base/ownership; preflight any metadata dependency before consuming. A failed candidate returns without touching active bytes or undo; `GM_NO_CHANGE` doesn't commit/increment. Do not silently set `active_config` to a synthetic per-layer key: preserve legacy config/identity except explicit corresponding update; separate recipe keys. Legacy request and BE request share one active controller, latest wins.

**Red native fixture:** initialise real native tracks with distinct `step_t` bytes at Step 5 and Step 48; shorten selected track to LEN=16 (other tracks have different data). Capture all 8x64 native cells and timing via `memcpy` into **test** arrays. Run RHY-only supported mutation and TIME-only, verify only allowed fields in selected active range, all dormant bytes exactly preserved; KEEP on OFF and KEEP outside LEN refuse if not representable; PLOCK/MOTION coordinates and bank bytes unchanged. TIMBRE-only fixture after T05 must not round-trip through export/conversion at all. `gm_native_export` intentionally empties beyond LEN; `gm_to_native` clears padding, so copying reconstructed cells wholesale would violate preservation. Test no-change, supersede during WRITING/READY, native context switch, STOP, stale fixed base, no change of other tracks on unsupported grids. Test on playing 16/12/15 native END longest pattern and stopped immediate; do not change original cue semantics.

```c
/* Native test helper: compare all bytes including dormant cells. */
step_t before[NTRK][NSTEP];
for (unsigned k = 0; k < NTRK; ++k) memcpy(before[k], trk[k].step, sizeof before[k]);
/* submit a real compatible BE mutation; poll and play actual END (cue test harness) */
assert(!memcmp(&before[2][16], &trk[2].step[16], 48 * sizeof(step_t)));
for (unsigned k = 0; k < NTRK; ++k)
    if (k != 2) assert(!memcmp(before[k], trk[k].step, sizeof before[k]));
```

- [ ] Run `be_runtime` RED; use real `gm_runtime_poll`/cue path, not helper writing results into live state.
- [ ] Build playable candidate in existing base/out buffers, validate before reusing union; once base lifetime ends copy **full native** source (all cells/timing) into `gmrt.u.n`, then patch named fields only in selected playable steps (`on,acc,slide,tie,rat,cond,nt` for RHY; `micro` for TIME). Skip byte writes to dormant/protected/unrelated cells. Verify every source-dependent read came from unchanged revision at READY; no long IRQ-off generation. For the new BE path, capture source bytes in bounded protected per-track or smaller slices; check expected native revision/context at each slice and again at READY. The legacy export path stays unchanged. Never validate or generate from a partially captured snapshot. When a native edit interrupts the sequence of slices, reject at revision check. No full pattern copy inside one new long IRQ-off window. Do not pass dormant native cells to portable core validator. TIMBRE-only stage has metadata-only marker, skips grid export, commit skips native copy (snapshot only if required for undo). The controller still owns stage/ack. Compare candidate native fields and semantic metadata; ignore proposed next key alone for NO CHANGE when musical result unchanged. If recipe's deterministic future behavior truly changes, include recipe identity as a semantic change; do not fake successful empty mutation. Apply only changed fields of patch; leave current live POS/ON/MOVE/endpoint contents untouched unless explicitly in write set. E.g., changing only CURV must not overwrite later MOVE. Recheck relevant dependency edits via shared `be_edit_isr` (T03).
- [ ] Green `be_runtime`, `adapter`, `cue`, `controller`, `golden`; mutation test that replaces dormant cells with converted zeros must fail. Commit `feat(grind): publish scoped native layer candidates`.

### T03 - Atomic undo, edit invalidation and context reset

**Files:** `grind/runtime.c`, `grind/be_runtime.c`, `core.h`, `seq.c`, `motion.c`, `midi_learn.c`, `project.c`, `cue.c`, `song.c`, `ui_input.c`, `ui_tools.c`, `editor.c`, `editor_pattern.c`, `editor_backup.c`, `presets.c` where audit shows actual writes; tests `test_be_runtime.c`, `test_cue.c`, `test_edit_sites.py`. Do not modify frozen files.

**Contract:** snapshot validity + identity + relevant basis/context + `chain.running` checked **inside the same IRQ guard** as cancellation and swap. Use monotonically advanced identity for new snapshot including replacement and context reset; read the intended identity in a short guarded observation at invocation, then revalidate immediately before swap (host race hook between guards). No unguarded multibyte snapshot reads. Rejected swap changes nothing itself (pending valid follow-live intent survives; edit-caused invalidation still takes effect). `gm_touch_isr` for unrelated native edits invalidates matching undo metadata and pending dependencies; undo uses a distinct internal revision bump that does not invalidate its own snapshot. Effective edit + invalidation in one critical section; audit legacy main writes using `gm_touch()` before writes, which alone is not an atomic edit transaction. Already IRQ-off/ISR paths use no nested `fm1_irq_on`. Non-effectful knob movement/failed edit doesn't invalidate.

```c
/* Shape of gm_runtime_undo after T03; helper names introduced in this task. */
fm1_irq_off();
const uint32_t wanted = be_undo_identity_isr(); /* observe currently offered snapshot */
const int observed_valid = gmrt.undo_ok;
fm1_irq_on();
/* host-only deterministic hook runs real rec_hit or replacement commit here */
fm1_irq_off();
if (chain.running || !observed_valid || !gmrt.undo_ok ||
    wanted != be_undo_identity_isr() || !be_undo_matches_context_isr()) {
    const int rc = chain.running ? -1 : 0;
    fm1_irq_on();
    return rc;
}
gm_runtime_stop();                /* only after successful validation */
/* existing per-track swap of steps/timing; skip for metadata-only history */
be_swap_undo_metadata_isr();      /* exact recipe/config/identity and owned fields */
gmrt.undo_was_undo ^= 1u;
be_undo_advance_revision_isr();   /* stale previews/stages; maintain own snapshot */
fm1_irq_on();
return 1;
```

`be_undo_identity_isr`, `be_undo_matches_context_isr`, `be_swap_undo_metadata_isr`, `be_undo_advance_revision_isr` are new native helpers in `be_runtime.c`. Existing `gm_ctl_native_edit` can be called by the dedicated successful-swap helper, with the same cue-clearing effect but no unrelated-edit invalidation. Use existing one-slot native swap, no history stack. Save changed-scope mask, matching old cfg/recipe and relevant lock metadata, current-at-commit live-field versions; swap only restored fields. Non-owned live fields remain live. Actual edits to fields that undo would restore or protected basis invalidate undo; unrelated MOVE/POS may remain live and must not be rolled back by undo. Store POS/ON/MOVE observation with snapshot for version/diagnostic purposes, without copying all endpoints into each undo; if a command explicitly changes a live field, that field's old value must fit and be swapped or the command is not a mutation/undo operation. Session import is a separately explicit state replacement, invalidates history (T14). Ordinary STOP/refusal/no-change preserve undo.

**Red races (real paths, not flag-only simulation):**
1. Valid undo, pending legacy `follow_live=1` request, blank REC target, `song.playing=1` and correct arm/source conditions. Test hook in pre-guard window invokes real `rec_hit`, records nonidentical hit, invalidates history. Undo returns 0, hit remains, no extra revision/toggle/cancel; follow-live intent remains pending and can subsequently build against changed base. A fixed-basis candidate may naturally become stale due to the recording; don't assert stale work must still commit.
2. Valid snapshot A; stage B ready; hook executes real native B commit, creates valid replacement snapshot, then enqueues new valid request C (after B, earlier enqueue would supersede B). Old undo invocation refuses; B's live pattern/timing, snapshot, identity, request C intact. Fresh undo observes B and restores B's predecessor. In separate control verify C can commit normally after refused undo; fresh successful undo intentionally cancels pending. Test mutant checking only `undo_ok` must fail.
3. Undo/redo success, then live REC; recorded hit stands, redo/undo expires. Manual Step, Timing, KEEP, layer lock, PLOCK, MOTION write, MIDI-learn/editor same-value vs actual edit, model/base edit, variation/slot/project/restore, INIT, SONG/context. Exact old/new config and recipe identity. PLOCK/MOTION bank never swapped by generation undo. Ordinary STOP preserves history; context reset invalidates; UI/editor sees actual valid flags. No newer recipe attached to newly loaded pattern.

- [ ] RED `be_runtime` and existing `cue`/`ui` tests for revised validity. Expand `tests/grind/test_edit_sites.py` for new protected metadata/base writes without weakening existing audit. Audit `seq.c:rec_hit`, `keys_rec`, tie/slide writes, `motion_add/capture`, MIDI learned CC, `editor_pattern.c` live vs offline writes, `project.c` native cue/load and all actual parameter edits. Update tests whose old expectation explicitly conflicts with the approved undo contract; keep legacy output goldens unchanged.
- [ ] Implement guarded state transition plus scoped invalidation and reset. `be_context_isr` clears active/pending recipe association and undo, invalidates preview/import/export, resets appropriate session defaults; use existing `gm_context_isr` and STOP paths. Do not use context reset for ordinary STOP. Keep pattern and metadata coherent before any ISR reader sees new state. Use distinct native-dependency and export-dirty flags (T14 increments export token also on unrelated live settings). Check all no-op and refused paths preserve state.
- [ ] Green `be_runtime`, `cue`, `persist`, `ui`, `editor`, `python3 -m unittest discover -s tests/grind -p 'test_edit_sites.py' -v`; ASan/UBSan native fixtures. Commit `fix(grind): make scoped undo and edit invalidation atomic`.

### T04 - LOCKS panel, preview and real interaction

**Files:** `grind/ui.c`, `pages.c`, `ui.c`, `ui_draw.c`, `ui_input.c`, `tests/ui_host.h`, `tests/sim_host.h`, new `tests/grind/test_be_ui.c`, runner. Extend actual existing GRIND scope or add an explicit BE scope with bounded descriptor IDs. Append, do not renumber GU or stored IDs. `page_desc` currently returns `int16_t*` for `gm_v`, so use a small selected-track display/command proxy and explicit guarded setter, never cast `uint8_t*` recipe fields to `int16_t*`. On track switch refresh proxy; encoder actions capture `song.sel` once.

- [ ] RED actual panel test: ARP navigation old pages present in old order, added LOCKS page with RHY/TIME/TMBR/MUT. Track A lock does not set track B; all locked shows LOCKED, no request ID/dirty/commit; held Step intercepts unsupported PLOCK, never invokes MUT; long KEEP behavior remains. TMBR is clearly unavailable until BE02, no pretend feature; if only unavailable TMBR is free, report unavailable instead of pretending a mutation succeeded. Preview compact and read-only, BUSY when stage/pending/import scratch owned. Playing shows PENDING END until ack, stopped applies once; refused/stale distinct; selected track/actual lock state displayed. Check old native recovery chords/BLAST IGNITION behavior and GRIND_NATIVE=0 has no new pages.
- [ ] Implement descriptor and routing for `LOCKS` on `FAM_GRIDS`, RHY/TIME are per-track bits, TMBR enabled in T07, MUT action resets after right detent and calls `be_submit(track, allowed_layers)`; no action from left detent or held-Step fallthrough. Keep old GU_MUT behavior. Render LOCKED/FREE, `MUTATION ONLY`, selected track and status in existing 240x124 Canvas. No bitmap per page, no full-frame allocation. UI proxy not active state, new field ownership tested. Both harnesses receive definitions.
- [ ] Green `be_ui`, `ui`, `menu`, `cue`, boot fixtures and web/native simulator build; actual LCD screenshot in each theme (not a separate design mock). Commit `feat(grind): expose per-track layer locks and scoped mutation`.

## 5. BE02 - TIMBRE DNA (T05-T07)

### T05 - Deterministic contours and timbre-only mutation

**Files:** `grind/be.h`, `be_math.c`, `be_runtime.c`, `tests/grind/test_be_timbre.c` (portable), `test_be_runtime.c`, runner. Use recipe fields, no 8x64xN parameter cache.

**Proposed v1 contour math:** normalized signed raw offset `[-127,127]`, phase `i=step%span` for span 1..64. `round_signed(n,d)` means `sign(n)*floor((abs(n)+d/2)/d)` for d>0; use bounded int32 values, no negative signed shift or unsigned wrap reliance. `span=1` ASCEND/DESCEND return 0. `amount` 0..100; effective offset `round_signed(raw*amount,100)`. For FM INDEX/MDEC/FBK use this signed raw offset in the 0..127 domain, clamp after addition. MOTIF four-phase `{-64,32,-32,96}`, PULSE first phase +127, others 0; ALTERNATE -127,+127. CHAOTIC uses stable `gm_mix32(key[2] ^ (track<<8) ^ i ^ 15u)` mapped modulo 255 minus127 (new reserved BE domain, check DNA allocation). Contour and range explicitly versioned. These numerical choices are proposals for v1, not measurements of BRUTAL FM.

```c
int16_t be_contour(const be_recipe *r, uint8_t track, uint8_t step);
/* returns amount-scaled raw delta; invalid recipe refused before publication */
int32_t be_round_signed(int32_t n, int32_t d);
```

Red fixture ASCEND span=5 amount=100 -> `{-127,-63,0,64,127}` if computing `-127 + round_signed(254*i,span-1)` (choose round-half-up positive numerator, so i=1 -> -63); DESCEND is negation `{127,63,0,-64,-127}`; choose one explicit formula and keep independently coded C/JS reference. Step 7 equals Step2 at span5. ALTERNATE span4 amount50 -> `-64,+64,-64,+64`. MOTIF sequence fixed; PULSE index0 only; CHAOTIC independent reference using explicit uint32 FNV-free mix algorithm from `rng.c`, golden values for fixed key/track/step, call in different orders; AMT=0 always 0. Range, max span, illegal curve/target/amount, key separation. Validate no change of rhythm if new hit before existing hit; same logical step retains value. Check TIMBRE-only no native grid export even with unsupported source/division, but selected generic FM only; other models refuse. `GM_NO_CHANGE` when all locks on/no semantic change. 

- [ ] RED `be_timbre`, `be_runtime` (all native step/timing bytes before and after, including dormant notes). Implement pure function with widened bounded multiply, no allocations. For deterministic MUT of selected TMBR: propose next key, use separate BE domains to pick one of six curves, supported target and a nonzero amount within 0..100, preserve span and manual base values. Reject no change; only accepted changed publish increments key. For v1 keep BRUTAL mode/intensity manual: TIMBRE mutation changes contour/target/amount/key only. Any later explicit mode/intensity mutation must have a declared write set, model compatibility and no silent invalidation of enabled MORPH; it is not required for these PRs. TIMBRE lock protects recipe during mutation, not deliberate knobs. No control of LFO phase or VARY RNG.
- [ ] Green `be_timbre`, `be_runtime`, `be_layers`, `golden`; WASM portable parity using independent reference. Commit `feat(grind): add deterministic per-track timbre recipes`.

### T06 - Hit context, early Ratchets and automation-safe FM parameter path

**Files:** `seq.c`, `drum_core.c`, `dm_fm.c`, `dm_dsp.c`, `motion.c`, `lfo.c`, `core.h`, new `grind/be_sound.c`, `grind/be_runtime.c`; new native `tests/grind/test_be_sound.c`. Inspect `seq.c:step_fire/rat_due/mic_early/mic_due/mic_enter`, `drum_core.c:drum_play`, `motion.c:motion_moved/lock_held/motion_slot/motion_rest`, `lfo.c:lfo_slot/lfo_hit`, `dm_fm.c:fm_trigger`.

Native roll latch proposal: per track `{uint8_t step,target,valid; int8_t delta;}` 4 B x8 (delta fits [-127,127]), plus bounded hit context passed only to the actual FM trigger; no new field on each voice. First `step_fire` latches the actual logical `seq_idx` (temporary next-step index when `mic_early`), and unscaled-by-MOVE contour offset/target. Ratchets use this latch even before `mic_enter` restores playing index; after roll cleared/STOP/context/model change reset validity. Direct input must not accidentally inherit the previous roll; explicit `drum_play` wrapper for sequenced hit vs direct path, no global stale flag surviving muted/safe-start early returns. The trigger occurs synchronously on the audio thread; use scoped hit args or short native context restored on every exit. Reuse existing trigger API by FM-only explicit helper or scoped local context; no new generic function-pointer ABI needed. Direct hits receive no DNA but retain live MORPH/BRUT. Treat rolled raw offset as stable until roll completion, MOVE live factor applied per hit.

**Parameter ownership:** For FM only, resolve eight local `int16_t` values just before actual `fm_trigger` coefficients, after HIT-LFO reset (`drum_play` calls `lfo_hit`). Neutral fast path `p=&t->p[P_E0]` unchanged. Do not write new base into `t->p`, `lsave`, `mo.base`, `mo.under` or lockbank. `motion_rest(k,id)` retrieves true base; `motion_moved(k,id)` and `lock_held(k,id)` determine whether a recorded absolute value overrides it; `*motion_slot(t,id)` is the effective pre-LFO value in that case. Do not infer presence from equal values (automation may equal old base while morph differs). Obtain the **unclamped** existing LFO contribution by the exact existing four-slot accumulation rules using current `lfo[].out` and depth, not `t->p - lsave` (clipped old output loses contribution). Factor calculation into helper shared with `lfo_slot` or equivalent side-effect-free same-order computation. It must neither advance phase nor call `lfo_track` again. BE06 shares the same scaling helper. Apply final descriptor clamp only once after contributions; when existing automation owns the absolute value, use it as center rather than applying DNA on top. Preserve original quantization/order for disabled path. FM TUNE must be used by actual `dm_tune` computation (read `dm_dsp.c`, preserve note/transposition/clamps); changing only local array while `dm_tune(t)` reads original `t->p` is not sufficient. Leave other FM render, sample-layer, LFO state and legacy drum models untouched.

```c
/* conceptual native helper; actual descriptor bounds from track_desc */
static int16_t be_resolve_fm_param(track_t *t, unsigned id, int16_t base,
                                   int32_t dna, int32_t lfo_delta) {
    const unsigned k = (unsigned)(t - trk);
    const param_desc_t *d = track_desc(t,id);
    int32_t centre = clamp((int32_t)base + dna,d->min,d->max);
    if (motion_moved(k,id) || lock_held(k,id)) centre = *motion_slot(t,id);
    return (int16_t)clamp(centre + lfo_delta,d->min,d->max);
}
```

Implement after relevant function definitions with early prototype in `core.h`; `dm_fm.c` appears before `motion.c`/`lfo.c`. No hidden mutable cross-thread pointer.

**Red actual playback fixtures:** adjacent Step 8/9 opposite contour, negative microtime with multiple ratchets both before and after boundary (choose micro/division/roll schedule that actually has hits on both sides; assert trace times independently). `fm_trigger` latched `v->x[0]` distinguishes INDEX. Assert every step-9 roll raw delta matches step9, not step8, and actual `v->x[0]` follows isolated no-automation fixture. Late/normal step control, direct MIDI during roll and direct REC monitor hit must not alter roll latch. Zero amount exact PCM before/after, same RNG/LFO states, no `mo.b`/base/lsave corruption. MOTION and PLOCK overriding the same target with a value equal to old base still override new base. LFO saturation fixture: old base 120, unclamped +30, new center 60 ->90, not67. Multiple same-destination slots sum with existing rounding and HIT phase behavior. `VEL=0` or velocity96 isolates INDEX; check other targets via actual `v->k[1]` and feedback coefficient, independent formula. TUNE used in actual `v->inc[0]` in T12. Safe-start/muted/choke no leaked hit context.

- [ ] RED `be_sound`. Implement roll latch and true FM trigger parameter resolution. Latch only once per step; use real native path, no test-only fake trigger. Add code comments specifying raw/scale distinction and model gating. Keep previous FM fast path for no enabled BE contribution. Audit early returns, roll cancellation, cue, STOP, model switches and host-only hooks. Use pure/side-effect-free math, not extra per-sample work.
- [ ] Green `be_sound`, `cue`, `vary`, `hit_variation`, `golden`, existing upstream DSP/audio fixtures; both GRIND variants. Commit `feat(grind): resolve deterministic timbre at the actual FM hit`.

### T07 - TIMBRE panel and scoped publish integration

**Files:** `grind/ui.c`, `pages.c`, `ui.c`, `ui_input.c`, `ui_draw.c`, `be_runtime.c`, `tests/ui_host.h`, `tests/sim_host.h`, `test_be_ui.c`, `test_be_runtime.c`.
- [ ] RED: generic FM page CURV/AMT/SPAN/TARG, unsupported models visibly unavailable, selected-track routing and all bounds, TMBR lock now functional. RHY/TIME locked, TMBR free -> MUT queued END and applied metadata only; all 512 step bytes/timing equal; actual hit value changes, not only display. Locked TMBR unaffected by RHY mutation; manual AMT change after pending TMBR invalidates candidate, unrelated UI selection doesn't. Stopped preview/apply uses same candidate, no request from preview; all locks locked -> no action. A failed/no-change mutation leaves key, undo, dirty flags intact. Manual control is visible live, not falsely labelled pending.
- [ ] Implement page appended to EDIT family with bounded local descriptor and explicit commit/guarded edit setter; selected track only. Small curve from `be_contour` over current span drawn using existing Canvas, sample at display columns not audio cache. Use text status, graph and trigger strip; no full framebuffer. Ensure CAP/MUT actions do not fall through from held step. Reset/unsupported model leaves no hidden metadata or live contribution. Extend preview token with matching recipe/track revision only where needed; legacy preview bytes unaffected.
- [ ] Green `be_ui`, `be_runtime`, `be_sound`, `ui`, `menu`, native/WASM UI fixtures; commit `feat(grind): add timbre page and nondestructive mutation workflow`.

## 6. BE03 - BRUTAL PERC (T08-T09)

### T08 - FM modes, latched state and bounded DSP

**Files:** `dm_fm.c`, `dm_dsp.c` if needed, `grind/be_sound.c`, `grind/be.h`, `core.h` only for hooks; `tests/grind/test_be_perc.c`, runner, `LICENSING.md`. Reference `firmware/brutal/bfm_dsp.c` at pinned BRUTAL FM commit; preserve attribution for any ported code. Inspect actual `dvoice_t` field ownership, allocator `dv_init`/zeroing, `drum_cut`, `fm_trigger/render`, DSP numeric helpers and target disassembly first.

The generic FM uses `ph[0..1],inc[0..1],env[0..1],k[0..1],x[0..3]`. Proposed existing free-field allocation (must confirm no shared helper reads them): `k[2]` packed mode/intensity, `ph[2]/inc[2]` GROWL sub only, `rng` NOISE separate from pattern RNG (already per voice), `f[0]` optional bounded state if needed. No enlargement of 444-B voice, no extra voice allocation; `dv_init` resets fields, active voice uses latched mode and coeffs after manual edits. Do not reuse `last`, `active`, `vel`, `age`, shared filter state or sample-layer memory indiscriminately. FM's render `x[3]` contains two feedback samples; preserve existing feedback history/layout. Legacy path exact when mode OFF **or** intensity 0, including normal FM ratio, phase, feedback, RNG, gain and output.

**Proposed initial mode formulas (deliberately bounded, tune after actual target measurement, not exact upstream port):**

- METAL: adjust modulator increment at hit using fixed-point factor `1 + intensity/(2*127)` (Q12, bounded 1..1.5); at high pitches cap the active METAL modulator increment at `UINT32_C(0x7fffffff)` (below Nyquist), computing the detuned and swept increment in a widened intermediate before narrowing; preserve the original neutral branch exactly. Keep existing legacy increment at neutral. No multiply/divide per sample for ratio change; test worst tune/RATIO/SWEEP.
- GROWL: optional sub at half carrier frequency using `ph[2]`, fixed-point blend with existing carrier `c`, weight 0..64/256 (<=25%). Sub uses same amplitude envelope; sweep behavior explicitly either follows `ic/2` each block or fixed documented fundamental, choose follows `ic/2`. Use widened/bounded signed product and conservative gain; no additional voice or envelope. Measure extra sinus/cycles first; preserve existing voice cap.
- CHAOS: use existing feedback average and a bounded odd soft-shaping curve (`q=x*32768/(32768+abs(x))` reference; implement equivalent bounded lookup/piecewise linear with no division in sample loop). Blend original average and shaped feedback by 0..127 intensity; clamp signed modulation safely, intentional unsigned phase wrap only. Intensity zero dispatches legacy exactly, not a permanently nonlinear path. Proposed four-segment piecewise `abs(x)<=8192: a; <=16384: 8192+(a-8192)/2; else 12288+(a-16384)/4` with sign and signed rounding; blend then use existing FBK scaling. No unbounded self-feedback gain.
- NOISE: per-voice existing seeded noise state, bounded phase perturbation of modulator, e.g. `noise_q15 * intensity * 64` phase units with widened arithmetic (maximum absolute <267 million). Use repository's verified noise generator, advance only in enabled NOISE, never touch Pattern RNG or global LFO RNG; no UB from signed overflow. Blend/add with unsigned phase accumulator only after bounded signed result.

These are independent new mode designs inspired by the reference; do not describe them as exact BRUTAL FM emulation. If a direct formula is ported, preserve its GPL notice. Use block precomputation and mode dispatch outside sample loop where practical. Test peak/DC/tail and compare native weighted cost; if GROWL or other path too costly, simplify coefficients/loop, not global cap or algorithmic audio scheduling. No global HQ/oversampling.

**Red:** render real FM triggers with fixed seed/velocity/note/base values in mode OFF and all modes intensity0; independent pre-change reference PCM and voice state equal for 1 and 2 overlapping voices, high feedback, sweep, voice steal, silence. For each nonzero mode prove audible/PCM change compared with fixed baseline, deterministic bounded output with same input, max/min params, tails terminate, no DC in silence, no corrupted neighbor voice, no new per-track global RNG. Change MODE/BRUT during ringing voice and trigger new one: first voice remains original mode, second uses new; model change stops old voices via existing model_follow. ASan/UBSan on new math without adding sanitizer exemptions. Example `assert(memcmp(pcm_off, pcm_zero_mode, sizeof pcm_off)==0); assert(memcmp(pcm_off,pcm_on,sizeof pcm_off)!=0);` with independent captured arrays, not same storage. Track sample-layer unchanged except normal mix.
- [ ] Run `be_perc` RED, then implement `be_fm_latch` and per-mode render with legacy branch unchanged. `fm_trigger` consumes resolved base/MORPH/DNA params and current latched MODE/BRUT. `fm_render` never re-reads them from live track. Check actual field occupancy and static assertions for voice size. Code attribution and `LICENSING.md` entry before first ported code. No copied non-GPL assets.
- [ ] Green `be_perc`, `be_sound`, existing upstream drum/audio/performance suites, `golden`, target Native build, `tests/target_budget.py` via existing CI. Report mode-specific target-cost delta, flash/app-RAM/stack. Hardware time OPEN unless actual measurement. Commit `feat(dsp): add latched bounded brutal FM percussion modes`.

### T09 - BRUTAL controls and recipe compatibility

**Files:** `grind/ui.c`, `pages.c`, `ui.c`, `ui_input.c`, `ui_draw.c`, `be_runtime.c`, both UI/sim harnesses; `test_be_ui.c`, `test_be_perc.c`. Add `EDIT -> BRUTAL` MODE, BRUT, TUNE, DEC. MODE=OFF/METAL/GROWL/CHAOS/NOISE; BRUT 0..127 as v1 parameter range, show meaningful percentage or raw consistent descriptor; no duplicate TUNE/DEC storage. TUNE/DEC use actual native base edit routing (including existing supported PLOCK behavior), MODE/BRUT explicit BE scope. No preset ID extensions.
- [ ] RED: selection/track isolation, non-FM unavailable, manual TUNE/DEC changes original base/appropriate native PLOCK, MODE/BRUT never routed to wrong P_COUNT or shared `gm_v` index, held Step cannot trigger unsupported action. Mode changes while sounding affect next trigger only. Context/model/preset resets defined, no hidden stale mode applied to other models. Manual BRUT changes invalidate only dependent pending/undo and future MORPH if captured base changes. V1 TMBR mutation does not own BRUT mode/intensity; newer manual BRUT fields remain live, unless a changed relevant dependency invalidates the candidate. No implicit disable of MORPH. Tests verify newer unrelated values preserved at commit.
- [ ] Implement guarded edit setters and page, verify actual renderer state. Unsupported model switching resets that track's BE sound recipe/endpoints (later T12) with visible status, pending invalidation. No flash writes. Use current LCD primitives.
- [ ] Green `be_ui`, `be_perc`, `be_sound`, `persist`, legacy UI/native/wasm. Commit `feat(ui): add per-track brutal percussion controls`.

## 7. BE04 - SINE FOLDER (T10-T11, independent branch)

### T10 - Fixed-point sine shape and explicit INSERT dispatch

**Files:** `core.h`, `insert.c`, optionally new `grind/be_sine.h` pure shape helper, `params.c`, `tests/grind/test_be_sine.c` native, runner, `LICENSING.md`. Existing `IT_OFF..IT_CHORUS` IDs stay identical. Append `IT_SINE` immediately before `IT_N` **under GRIND_NATIVE**, ensuring non-native `IT_N` and accepted values unchanged. For native old values 0..8, new SINE=9 if still free at implementation; verify enum/consumers on actual main. Existing `ins_t`, `.pool` `ins[8]` 4.640 B, no extra delay lines. Explicit dispatch SINE before `>=IT_PHASER`; audit all such classification in params, graph, persistence, editor and any labels. Preserve old FOLD.

**Proposed pure shape:** use existing `sine_i` phase convention (uint32 full cycle), no new LUT. Input `v` clamped to signed range [-230000,230000] as existing drive, normalize for phase, FOLD gain from 1x..4x in bounded Q12, SYMM controls positive/negative gain in bounded range (choose Q12 +/-25% around center), BIAS phase offset +/- quarter-cycle. Form phase via widened signed arithmetic then intentional unsigned modulo. `f(v)=(sine_i(phase(v)+bias)-sine_i(bias))/2` (range signed Q15), exact f(0)=0 regardless of bias/symmetry. Use conservative gain, no full identity claim at FOLD=0. Block compute gains and `sine_i(bias)` once, per sample bounded multiply/shift, DC blocker and existing wet mixer. Implementation reference shape with explicit phase scaling:

```c
/* q12 gain chosen for input sample scale 32768; phase quarter-cycle at v=32768 and gain=1 */
int64_t phase = (int64_t)v * gain_q12 * 8; /* 32768*4096*8=2^30 */
int32_t y = (sine_i((uint32_t)phase + bias_phase) - bias_sine) / 2;
```

Use target-friendly bounded alternative to unnecessary 64-bit sample-loop multiply after proving equivalence (phase modular product can use unsigned 32-bit multiply). No signed-shift UB. Reuse wet fade and DC state; note `track_insert` chooses old `s->type` while ramping to new type. Do not reinterpret old SINE A/B/C as newly selected types during fade: latch the three last valid SINE A/B/C bytes per track (24 B total before alignment) when the selected and effective types agree, then recompute outgoing SINE block coefficients from those bytes during fade; clear validity when the existing effect instance resets, count bytes; for outgoing legacy behavior preserve old implementation unless required by new SINE integration. At zero wet clear old state before applying new type. For OFF or fully dry MIX=0 output bit-identical input; transition with nonzero wet intentionally fades.

**Red fixtures:** `f(0)==0` for bias/sym min/center/max, independent sine reference tolerance agreed from existing LUT, monotonic near small signal/actual folding over range, gain/peak numeric bounds. Feed exact zero from rest at high wet; no synthesized DC or click. Positive/negative asymmetry extremes, impulse then long silence/DC, max input and control values no overflow. Old types 0..8 sample goldens identical; SINE not reaching swept path (verify state/outputs), new type->OFF and reverse, SINE<->CRUSH/PHASER quick changes, silent tail idle clears, no residual filter/line on new hit. Validate actual wet ramp, not only descriptor. Test DRUM/native non-GRIND build cannot select SINE.
- [ ] RED `be_sine`. Implement dedicated explicit SINE shape/render using existing `ins` instance; branch outside old loops. Reuse existing DC blocker and `pf_mix` smoothing, compute f(0) compensation, ensure output peak and idle behavior. Add FOLD/SYMM/BIAS descriptors at effective type with correct bounds/defaults (existing A/B/C are 0..127), append name; `ins_desc` must not route new ID as swept. No new `.pool` storage by default. Record actual DSP target-cost and map.
- [ ] Green `be_sine`, `master_fx`, existing upstream insert/DSP/legacy suites, native `target_budget`, sanitizer. Commit `feat(dsp): add zero-centred sine insert without new effect buffers`.

### T11 - UI, stored type, protocol and compatibility

**Files:** `params.c`, `ui_draw.c`, `pages.c` (existing INSERT pages), `project.c`, `presets.c`, `editor.c`, `editor_pattern.c`, `editor_backup.c`, web `editor/proto.mjs`, `device_data.json`, `tab_sound.mjs` where consumer audit requires; `test_be_sine.c`, `test_persist.c`, `tests/editor_test.c`, `web/editor/test_editor.mjs`.
- [ ] RED: actual panel TYPE=SINE, A/B/C FOLD/SYMM/BIAS and graph calculated by same pure shape (not swept graph), existing MIX/SRR page. SINE native save/load via existing type field, preset/variation/project/backup/restore path accepts appended valid type and rejects invalid IDs consistently. Old saved types load exactly; old firmware not promised to play new type, old editor must not write out-of-range values; non-native build old max/labels. New editor descriptor bounds read from device, no reinterpretation of legacy commands. All old golden bytes unchanged. Test `GRIND_NATIVE=0` with old persisted types.
- [ ] Audit every `IT_N`, `IT_PHASER`, `P_ITYPE`, `N_ITYPE` and generated/device descriptor table via `rg`; update only needed consumers. If old format explicitly excludes new type, use compatible existing field and validation only (no field layout/ST_MAGIC change without proof/authorization). If native storage cannot preserve new type safely, refuse explicit save/use as appropriate and label SESSION, not silently map to unrelated effect. Preserve freeze rules. Graph uses same compensated shape, avoids extra Canvas allocation. SINE labels and compatibility limitation in manual/README.
- [ ] Green `be_sine`, `persist`, `editor`, `ui`, both firmware variants, Node editor tests, CI license/assets. Commit `feat(grind): expose and roundtrip the appended sine insert type`.

## 8. BE05 - SOUND MORPH und Session (T12-T14)

### T12 - Compact endpoints, integer interpolation and actual trigger

**Files:** `grind/be.h`, `be_math.c`, `be_sound.c`, `be_runtime.c`, `dm_fm.c`, `dm_dsp.c`, `tests/grind/test_be_morph.c` portable, `test_be_sound.c` native, runner.

```c
typedef struct {
    int16_t p[8];   /* TUNE DECAY INDEX RATIO MDEC SWEEP FBK VEL */
    uint8_t model, mode, brutal, flags; /* bit0 valid */
} be_endpoint;
typedef char be_endpoint_is_20[sizeof(be_endpoint) == 20 ? 1 : -1];
/* only active endpoints [8][2] = 320 B; pos/on [8][2] = 16 B */
int be_morph_compatible(const be_endpoint *a, const be_endpoint *b);
int16_t be_lerp_i16(int16_t a, int16_t b, unsigned pos);
```

v1 all continuous FM knob values interpolate in their native parameter domain with signed round-nearest/ties away from zero of delta: `a + be_round_signed(((int32_t)b-a)*pos,100)`. TUNE is semitone integer (frequency naturally exponential via existing tuner, do not linearize Hertz); time/decay parameters use existing lookup law; RATIO requires exact equal enum, model `DM_FM`, same BRUTAL mode. VEL is continuous amount but excluded MOVE, can be morphed (isolated endpoint includes all eight). BRUT intensity interpolate, MODE discrete same. POS=0/100 exact, A=B unchanged, clamp only validated input to actual descriptors; invalid field or flag rejected before enabling ON. No implicit interpolation of different RATIO/model/mode. Direct hit and sequenced hit use same morph resolver, DMA sample-render reads only latched per-voice state. No tail morph in v1.

**Red portable/native:** A INDEX20 B100 POS37 ->50 (round 29.6 to30); A=-24 B=24 POS37 -> -6 (round17.76 to18); descending symmetric test 100->20 POS37 ->70; POS0/100 exactly, equal endpoints, unsupported RATIO mismatch/model/BRUT mode, invalid ranges. `fm_trigger` voice `v->x[0]` =50 at velocity96 with DNA/MOTION/PLOCK/LFO off, not only interpolation helper. TUNE changes real oscillator `inc[0]` using independent expected `inc_tune` formula based on note and resolved tune. With PLOCK/MOTION override and summed LFO, exact precedence tested; during ringing voice change POS new hit differs, old `v->x/inc` unchanged. With disabled morph and no DNA/BRUT exact old output. Capture at active MOTION/PLOCK/LFO returns resting values from `motion_rest`, not transient. A/B with same discrete fields and distinct BRUT intensity. POS/ON per-track isolation and no hidden state change if capture validation fails.
- [ ] Run `be_morph` and `be_sound` RED. Implement pure lerp and compatibility; store active-only A/B + pos/on. CAP uses one guarded bounded copy of 8 resting FM values + base BRUT fields/model; flags only after validation. Manual actual base parameter edit (UI/editor/MIDI/preset) disables ON for track, preserves compatible endpoints as design; incompatible model change clears affected endpoints. Mode mismatch must not be enabled unnoticed. Re-capture while ON: if endpoints cease compatible disable ON and show mismatch; compatible capture changes next hit. No automatic recording/flash. Add revision notifications for actual changes. Use resolver from T06; passing local TUNE to actual tuner required.
- [ ] Green `be_morph`, `be_sound`, `be_perc`, `vary`, `golden` and WASM portable parity; commit `feat(grind): add compatible hit-latched FM morphing`.

### T13 - MORPH page and race-free active live ownership

**Files:** `grind/ui.c`, `pages.c`, `ui.c`, `ui_input.c`, `ui_draw.c`, `be_runtime.c`, `tests/ui_host.h`, `tests/sim_host.h`, `test_be_ui.c`, `test_be_runtime.c`.
- [ ] RED: POS, CAP A/B, ON page; capture action one right detent reset, no held-step fallthrough; EMPTY/READY, MODEL MISMATCH, actual selected track. ON cannot be activated with missing/incompatible endpoints. Manual captured base edit disables ON, A/B still available. Stage TMBR mutation prepared with POS20, pending user sets POS80 and ON/MOVE changes when available; eligible candidate still commits and newer values remain. If mutation depends on modified base/recipe/lock or mode, reject as STALE. Test undo after mutation only swaps owned fields, does not restore old unrelated POS. If a new live edit touches a field actually restored by snapshot, undo invalidated. Recheck last selected track/guarded setters. Display session state and next-hit semantics, not END for POS.
- [ ] Implement descriptors and guarded UI setters, shared `be_edit_isr` routing. Session-owned live fields not in normal TMBR patch; undo observes current-at-commit versions, swaps only owned fields. POS/ON increments session export token (T14) even when native dependency unchanged. CAP/ON changes affecting mutation dependency invalidate candidate; don't treat every display selection as data edit. Keep shared fields synchronized when read by Audio ISR. No independent full DRAFT.
- [ ] Green `be_ui`, `be_runtime`, `be_morph`, `be_sound`, UI snapshots and both harness builds. Commit `feat(ui): expose morph and preserve live controls across staged commits`.

### T14 - Coherent Session Export/Import and editor protocol

**Files:** create `grind/be_session.c`, `web/editor/be_session.mjs`, `web/editor/test_be_session.mjs`, `tests/grind/test_be_session.c`; modify `editor_proto.h`, `editor.c`, `editor_pattern.c` if shared read helpers, `be_runtime.c`, `core.h`, `felucca.c`, `web/editor/proto.mjs`, `session.mjs`, `tab_grind.mjs`, `mock_device.mjs`, `tests/editor_test.c`, runner. Reuse existing DNA export token pattern but do not require T36 already implemented. Current wire protocol minor4, cap0x100, commands28/29. DNA plan reserves cap0x200, commands30/31, minor5. Proposed BE reserve cap0x400, explicitly numbered GET32/SET33, minor >=6 **only after capability actually available**; recheck all IDs at start, DNA may land first. Preserve old INFO/read semantics, never infer capability merely from minor. Handle enum gaps/dispatch bounds and duplicate SET detection. Max 640 bytes request, 400 bytes reply, existing `ED_PIECE=256`, 7-bit packing and u35 fields. No change of underlying USB/OTA framing.

**Ownership/token:** new `session_revision` increments on actual changes to **all exported bytes**, including pattern, dormant native step changes, timing, global/tempo, resting parameter/base, stored PLOCK/MOTION entries/play flags, KEEP/layer locks, model/preset assignment, context, actual CAP endpoints/POS/ON/BRUT/MOVE, recipe commits/undo. No increment for transient LFO/modulated render value, playhead or read-only preview unless that value is explicitly in export (not in v1). Pair `{context_epoch, session_revision}` using two u32 values in existing u35 wire encoding; capture/check under the existing IRQ guard; advance epoch on context or revision wrap, invalidate outstanding transfers on reset/disconnect/INFO re-handshake, never allow same-session stale token after wrap. Native dependency revision separate so unrelated live MOVE/POS won't cancel scoped mutation. Device verifies token in same short protected read as copying each bounded piece, **not** host-only check before unprotected getter; final status rechecks token immediately before sealing host package. Old ED_DUMP/STEPS_GET can expose a live read but are not automatically coherent with new metadata. Use new BE read ops for canonical live basis, not saved-slot fallback. Avoid long IRQ guards: one small piece <=256 bytes, check before/after, encode/send outside guard. Audit all exported-data writers; each actual edit advances token in same transaction. Playback only changing transient modulated parameters does not spuriously stale transfer. If source cannot be made consistent, abort rather than output mixed file.

**Canonical metadata v1 `BEV1` proposal:** little-endian explicit bytes, header 16 B: magic[4]='BEV1', version u8=1, tracks u8=8, flags u16=0, total_len u16=576, MOVE u8 (0..200, initially 100 until BE06), reserved u8=0, record CRC32 u32 (IEEE, over header with bytes 12..15 zeroed plus payload; covers MOVE and flags). Payload: 8 recipes x20=160, 8x2 endpoints x20=320, 8 pairs POS/ON=16, KEEP mask64. Canonical payload is **160+320+16+64=560**, header length 16+560=**576 B**. Store actual 64 native KEEP bytes including dormant positions; endpoint invalid entries canonical all-zero; known flags only. No struct memcpy to wire; explicit reads/writes, signed p as LE i16, canonical reserved zeros. Validate magic/version/size/CRC/each field/enum/flag/span/model compatibility (ON requires compatible valid endpoints matching current generic FM), MOVE range, KEEP coordinates and dormant native data preserved; do not reject legitimate dormant KEEP unless requested operation requires representable core constraints. Unknown schema fail, no partial updates. A/B ON/OFF with same endpoints and different POS must be distinguishable. CRC detects corruption, not cryptographic authenticity. For the v1 format use CRC-32/ISO-HDLC (reflected polynomial 0xEDB88320, init/final xor 0xffffffff; check vector `123456789` -> `0xcbf43926`). Check CRC before applying any fields, including MOVE in header. This revises the prior proposed payload-only checksum; there is no shipped BEV1 format to migrate. Host wrapper stores a complete canonical live **basis** with track models and resting params, global settings, all 64 native Steps/timing, active lock/motion events + playback flags and referenced sample/preset identity where relevant. No per-block transient or audio-tail state. Canonicalize order/IDs and version; host must verify referenced sample assets or label sound reconstruction conditional. For base matching, compare canonical basis bytes on host against a fresh device read under token; not merely slot number/hash. User deliberately restores differing base separately with existing project workflow, then verifies matching read. Device validates meta against current model/timing/keep coordinates and exact guarded token; never trusts arbitrary wire field lengths or uses client token to bypass compatibility. No auto writes.

**Wire proposal (op and token fields explicit, append-only):**

| Command | op | Arguments (after cmd/seq) | Reply semantics |
|---|---:|---|---|
| BE_GET | 0 STATUS | op | token(context u35, revision u35), supported schema, meta length u14, slot/var u7, busy/status |
| BE_GET | 1 READ_META | op, token, offset u14, count u14 (1..256) | same token + offset/count + pack7(meta slice) |
| BE_GET | 2 READ_BASE | op, token, section u7, track u7, offset u14, count u14 (1..256) | token + section/track + offset/count + pack7(slice) |
| BE_GET | 3 END | op, token | current token equal or STALE, no write |
| BE_SET | 0 BEGIN | op, matching token, total u14=576, CRC u35 | owns scratch if stopped, free and context matches |
| BE_SET | 1 DATA | op, token, offset u14, count u14, pack7(raw 1..256) | accept sequential bytes, exact duplicate retry idempotent |
| BE_SET | 2 COMMIT | op, token | validate all bytes, explicit atomic apply or refusal |
| BE_SET | 3 ABORT | op, token | release own scratch, no live update |

READ_BASE sections v1: 0 GLOBAL (`track=ED_NONE`), 1 TRACK_PARAMS (one track; count and signed i16 LE values including native timing and model, only rest values), 2 TRACK_STEPS (64 canonical 4-byte cells incl micro/dormant, not bitfield padding), 3 LIVE_AUTOMATION (sorted actual LK keys and vals, play mask, total count, no transient held values), 4 REFERENCES (selected necessary asset/preset IDs plus verified availability/content identity via existing read facilities; if not verifiable export indicates dependency and no self-contained audio promise). Canonical section layouts (all integers LE): section 0: `u8 schema=1, u16 G_COUNT, G_COUNT * i16` in native global-ID order; section 1: `u8 schema=1, u8 track, u16 P_COUNT, P_COUNT * i16` in native parameter-ID order; section 2: `u8 schema=1, u8 track, u16 count=64`, followed by 64 records `[flags u8, cond u8, note i8, micro i8]` in native step order (timing comes from section 1, flags map via `gm_from_native`); section 3: `u8 schema=1, u8 motion_play_mask, u16 event_count`, followed by exactly `event_count` records `[key u16, kind u8 (0 motion, 1 lock), value i8]`, in existing `(key,kind)` order, `event_count<=LK_MAX=1024`. Section 3 length is `4+4*event_count`; every slice walks bounded consecutive events, not the entire bank under IRQ-off. Section 4: host-side reference inventory derived from canonical model/sample selectors in section 1 and actual existing sample/preset read facilities; optional identity entries are explicitly marked verified/unverified rather than silently assumed. Do not advertise asset-complete restoration if reference contents cannot be verified. In v1 section 4 device read may return an explicit unsupported status, not fabricated bytes. Validate actual `P_COUNT/G_COUNT` on both ends and reject a different parameter map; no implicit schema migration. No serialized pointer, no uninitialized padding. Device slices generated from actual validated section lengths; reject invalid section/track/offset/count, overflow via `count <= length-offset`. `READ_BASE` live only, refuse chain/restore/OTA if coherent live basis unsupported. `READ_META` metadata bytes may be generated on demand with guarded small copying; no second full persistent snapshot. Pack7 worst-case 256->293 + headers below 400; independently test actual frame lengths, reserved command slots and malformed 7-bit bytes. At STATUS compute record CRC32 across bounded slices (including the canonical zero-CRC header), each copied under matching token, with CRC calculation outside the guard; cache only token and CRC (not a second snapshot). Recheck the token before returning STATUS. READ_META serves header CRC from that token or returns STALE; BEGIN CRC is the expected record CRC, checked against both header and computed record. `BEV1` CRC must correspond to exact tokenized header and payload; if changes during calculation return STALE; avoid rescanning full live state in IRQ. Test single-bit corruption of MOVE and of an endpoint independently, both with otherwise valid field ranges. No new native protocol buffer beyond existing ED and reused scratch.

**Import scratch:** borrow first 576 bytes of existing `gmrt.out` object representation (2144 B), only when no generation/preview/ready/reading/pending and transport stopped/no PLAY queued/chain/cue/restore/upload/OTA. One explicit scratch owner with token/received count/CRC and idle timeout via existing main-loop editor timing, no heap. Preview or new mutation while import active returns BUSY or explicitly aborts owned import before using scratch; STOP/context/disconnect/OTA/timeout aborts without data change. Duplicate retry does not append twice. Never let a new request overwrite `gmrt.out` while import validates. No `gm_pattern` accessed while scratch bytes owned. Validate complete metadata outside IRQ, recheck owner, token, context, stopped state and no pending work in guarded final publish; copy only validated compact fields under bounded guard; do not add another full temporary meta buffer, decode bounded prevalidated fields from owned bytes. Release owner on all paths. Apply resets pending/undo/preview as an explicit unrelated state replacement, revision advances once and no Flash write. Host verifies actual readback via new token. If base differs, no implicit load. At BE05 MOVE fixed 100 until BE06; don't advertise full bundle including mutable MOVE before final integration. Store only supported schema, reject nonneutral MOVE before implementation rather than silently drop.

**Red C/wire + independent JS fixtures:**
- Distinct compatible A20/B100 INDEX, POS37 ON and OFF, base INDEX10, velocity96. Before import set different POS/ON and endpoints. After apply, panel shows37, endpoints restored, actual FM `v->x[0]` =50 ON and10 OFF; OFF->ON without recapture returns50. Separate equal A/B invariant. Mutants resetting POS0/50/100 or updating display only or ignoring ON fail. Invalid ON missing/incompatible endpoint rejected without changes.
- Token changes between reads after commit, live POS/CAP/MOVE edit, manual base edit, slot/var switch, Live REC, automation edit; change after final chunk but before END -> no file. Change mid protected chunk -> coherent piece or STALE, never mixed. No-change action/preview/playhead/LFO evolution do not increment export revision. Explicitly verify writer mutation causes changed bytes/token. Wrap/context reset, malformed version/length/CRC/mask/enum/reserved, out-of-order/overlap chunks, duplicate retry, timeout/cancel, busy generation/PLAY/restore, malicious bounds and wrong token -> no partial live write or flash. Existing old ED_GRIND frames unchanged, unsupported firmware no action in host UI, legacy editor still works.
- Native C and JS codec bytes independent golden, pack7 worst lengths, every accepted SET checked for duplicate suppression, one apply ack. Base mismatched -> no import request; reconnect/disconnect invalidates stale token. Include exact dormant cells and automation data in basis comparison.

- [ ] RED `be_session`, `editor`, `node web/editor/test_be_session.mjs`. Keep direct session-code fixtures in `test_be_session.c` with real native runtime; add actual framed SysEx tests to `tests/editor_test.c`, which already includes the USB/editor harness. Do not claim wire coverage from the direct helper suite. Implement codec and revision helpers, bounded protected reads, shared scratch owner/abort lifecycle and native dispatcher. Use `ed_write`/existing retry classification for SET; ensure read-only GET doesn't change native state. Implement host `readStatus -> read chunks under token -> END -> save` and matching-base verification before `BEGIN -> DATA -> COMMIT -> readback`. File is local user download, no HTML from untrusted imported strings. UI shows SESSION, stale/busy/mismatch and explicit Apply. Test exact independent wire bytes, not mock-only correctness. If code cannot prove complete canonical base/reference support, keep capability for explicitly limited meta transfer only and do not claim full performance restore; complete promise is contingent on actual end-to-end evidence.
- [ ] Green `be_session`, `be_morph`, `be_runtime`, `editor`, `persist`, Node session/editor suite, `host-core/native/wasm`; target map records scratch ownership and metadata byte layout. Commit `feat(editor): add revision-bound brutal evolution session transport`.

## 9. BE06 - MOVEMENT und integrierter Abschluss (T15-T17)

### T15 - Mode-aware movement and neutral legacy path

**Files:** `grind/be_math.c`, `be_sound.c`, `be_runtime.c`, `lfo.c`, `params.c`, `insert.c` for effective-type helper, `core.h` prototypes, `tests/grind/test_be_move.c` native and `test_be_sound.c`, runner. MOVE global `uint8_t` or existing `int16_t` proxy, 0..200, 100 neutral, reset to100 (INIT/context defaults and old projects). It scales only modulation contributions, not base, POS/ON, endpoint data or stored LFO depths, phases, rates, random states. Actual existing `lfo_dest_param` mapping in `lfo.c:37-47`; R.TUN special dest11 (no param). Use effective descriptor and positive allowlist, not raw ID alone or `F_INT`/`names==NULL` alone.

| Target group | Allowed in v1 |
|---|---|
| FM (only DM_FM) | P_E0/E1/E2/E4/E5/E6; not RATIO E3 or VEL E7 |
| Mix | P_LEVEL, P_PAN |
| Resonator | R.TUN fine-pitch path, DECAY, MIX, TONE, POSITION; STRUCTURE only STRNG/PIPE/MODAL, no CHORD/OFF |
| Filter/FX | P_FCUT, P_FRESO, P_SRR, DIST, CHO/DLY/REV sends, P_IMIX |
| INSERT A/B/C | drives and swept modes with actual continuous descriptors; CRUSH only C=LPF, exclude A=BITS/B=RATE; SINE FOLD/SYMM/BIAS only when descriptor and actual DSP meaning continuous; OFF and ambiguous fade-out type no scaling |
| DNA | INDEX/MDEC/FBK raw contour delta, same factor applied before automation |

Do not scale VARY, SINK, SWELL, CUT, gate/velocity, clock/micro/swing, LFO speed/phase, discrete choices or unsupported model parameter. Existing routes to excluded destinations preserve legacy behavior **even at 0%**. Audit SRR and other approved descriptor values against actual DSP; quantized selector semantics excluded if they are discrete choices (not ordinary finite knob resolution). When selected and effective INSERT type differ during fade, leave A/B/C contributions legacy until unambiguous, without using incorrect descriptor. Expose effective type from existing `ins` state via read-only helper (include order check; no extra array). `P_IMIX` remains allowed independently. Keep R.TUN special path scaling once, using same factor for actual result and display; preserve existing intermediate integer rounding at unity.

```c
/* bounded contributions; use widened n if actual bound requires it */
static inline int32_t be_scale_delta(int32_t delta, unsigned move) {
    if (move == 100u) return delta;
    if (!move) return 0;
    return be_round_signed(delta * (int32_t)move,100);
}
```

Apply scaling to the summed per-destination contribution before final `lsave + delta` clamp. For R.TUN compute existing per-slot signed fine pitch sums and scale once; 100 uses exact original evaluation. FM hit resolver T06 calls the same unclamped-sum+scale helper; do not scale twice because `t->p` already contains MOVE result. At 0 active LFO routes still advance phase, random, freshness; do not change `LS_ON`/used bits, depth storage or reset phase. Amount zero/100% default remains exact. If product range multiplication may exceed int32, use target-appropriate widened bounded implementation or scale at safe accumulator range; no signed wrap.

**Red:** actual LFO slots with two contributions (+30 and -10), base60 -> MOVE0/100/200 yields60/80/100; negative and saturation cases independently computed with actual existing slot rounding, same LFO state/phase/RNG between MOVE0 and100 controls over identical blocks, resume at100 follows reference. CRUSH A/B, CHORD STRUCTURE, FM RATIO/VEL, non-FM enum destinations unaffected 0/100/200; CRUSH C and MIX scale. Type change with old effective mode during fade not misclassified. R.TUN fine pitch, generic FM hit resolver no double scaling, DNA offset scaling before PLOCK/MOTION precedence. Enabled LFO whose contribution saturates old value: new center uses raw sum. MOVE100 exact old audio/state, MOVE0 not muting source. VARY and master macros unchanged. INIT and old project read default100; slot/context reset documented; live MOVE changes session token but not unrelated pending mutation. 
- [ ] Run `be_move`, `be_sound` RED. Implement positive allowlist plus factor path in `lfo_slot/lfo_track` and resolver; if feature disabled or unity, take original arithmetic path. Read `song`/track state under existing ownership; MOVE set atomically with session token on UI/editor path. Do not introduce per-sample processing. Record target-cost deltas (`lfo_track`, `fm_render`, IRQ etc) and static state.
- [ ] Green `be_move`, `be_sound`, `be_morph`, `be_sine` if landed, `master_fx`, `vary`, `hit_variation`, upstream DSP/native CI. Commit `feat(grind): scale approved modulation contributions with movement`.

### T16 - LIVE page, full Session semantics and user documentation

**Files:** `grind/ui.c`, `pages.c`, `ui.c`, `ui_input.c`, `ui_draw.c`, `be_runtime.c`, `be_session.c`, editor UI/protocol JS, both UI host harnesses, `test_be_ui.c`, `test_be_session.c`, Node test. `docs/manual/` relevant pages, `README.md`, `LICENSING.md` if attribution of reference-derived MOVE formulas.
- [ ] RED: existing fourth GRIND page becomes LIVE, fourth knob MOVE, SINK/SWELL/CUT unchanged. Global scope visible (ALL), 0/100/200%, neutral reset100, no accidental Step/KEEP/PLOCK action. Capture session while MOVE20, change to80 before commit of unrelated mutation -> remain80; export started at20 and changed to80 -> STALE; full BEV1 export/readback MOVE80 including POS/ON, endpoints, recipe, KEEP and basis. Older capability missing -> no unsupported controls. After native SAVE and power cycle no silent claim that extra recipe is stored; old projects play with neutral defaults. Reset changes session token. Exact `GRIND_NATIVE=0` old UI/audio behavior.
- [ ] Implement UI proxy + setter (not second source of truth); update `BEV1` MOVE field actual runtime and validation, no layout bump since defined in T14 with neutral constraint; if interim capability/version did not support MOVE, explicitly advertise allowed features or bump compatibility as needed. End-to-end editor UI shows available capability/session semantics and uses actual readback. Update manual paths after listing `docs/manual/` current files; screenshots both themes, no image assets added unless warranted. Document existing effect/preset compatibility limitations and explicit matched-base import procedure. No automatic native persistence of recipes/endpoints.
- [ ] Green `be_ui`, `be_session`, `be_move`, `editor`, `persist`, Node tests, native/WASM preview; commit `feat(ui): integrate live movement and session controls`.

### T17 - End-to-end, regression and target resource ledger

**Files:** extend `tests/grind/test_be_runtime.c`, `test_be_sound.c`, `test_be_ui.c`, `test_be_session.c`, `test_be_move.c`, `test_be_perc.c`, `test_be_sine.c`, `web/editor/test_be_session.mjs`; add `docs/superpowers/specs/2026-11-28-be-evidence.md` at execution with actual per-PR SHA/evidence (not a fabricated results file). No separate global gate; finish BE06 with existing CI and specific integrated fixtures.

- [ ] RED integrated regression: prepare a valid Blastbeat with dormant native Step48 on selected track before LEN16; save all native steps/timing in test fixture; RHY/TIME locked, TMBR free; request mutation playing; only native END publishes; actual INDEX/MDEC/FBK changes while all 512 native cells and timing bytes identical. Undo/redo at once restore recipe+pattern identity; then extend LEN64 dormant note/attributes still intact and undo expired after edit. New mutation + manual step edit -> undo refuses, REC hit remains. Negative-microtimed Step9 roll with different Step8 raw offset; direct MIDI hit during roll not contaminated. Stage mutation, set POS80/MOVE0 and later100, ensure newer live controls survive. Confirm active BRUT old voice unchanged, SINE fades to dry, old insert types unchanged. Project/variation/slot switch clears affected session recipe, endpoints/identity, cancels stale candidates, new context does not resurrect old values; ordinary STOP preserves valid history. Verify active recipe matches ack and no unintended dirty/flash actions. Host bundle with distinct A/B, POS37 ON/OFF, MOVE and compatible matched basis roundtrips; mid-export commit/live edit rejects. Invalid import no partial change. Native save/load old records unaffected. This scenario must fail on at least targeted mutant from each new contract, not simply assert final UI labels.
- [ ] Implement only remaining integration fixes demonstrated by tests, not scope expansion. Verify common `D05` hooks documented in DNA implementation plan handoff, no duplicated controller/undo or cap collision. Compare exact changed files against intended PR base and run `git diff --check`. For each feature-off control run old PCM and golden tests, both build variants. Collect target linker map/size/symbols, native resource report, actual static stack call paths, target disassembly/cost and CI outputs for final commit with same build flags; compare net changes vs BE00 and previous merged step. Record **measured** RAM/Pool/Flash/NOINIT/RAMTEXT/stack and separate estimated future allocations. Check native 98.304 cap, 8.192 floor, frozen files, existing voice cap. If measured total >98.304 stop/report per AGENTS, no silent optimization of unrelated features. Record real device traces when available using BE00.2, otherwise hardware OPEN. Distinguish audio sample time from control-block cost and weighted static instructions. No hardware claim based on host/WASM.
- [ ] Run complete existing lanes in section 3.1, plus `sh tests/run_grind_tests.sh be_layers`, `be_runtime`, `be_ui`, `be_timbre`, `be_sound`, `be_perc`, `be_sine` (if merged), `be_morph`, `be_session`, `be_move` as **separate one-arg invocations**. Node editor/session tests, legacy/manual compatibility, no new gate. Commit `test(grind): verify brutal evolution integration and resource evidence`.

## 10. Traceability, Review und Rollback

| Anforderung aus gemergter Spec und Review | Aufgabe / konkreter Nachweis |
|---|---|
| Dormante Steps, alle 64 native Zellen | T02/T07/T17 byteweise Tests, native Erhaltung vs portable export |
| Undo Lifetime nach fremden Edits | T03/T17 realer Schreibpfad, genau ein Snapshot |
| Undo Guard inkl. Live-REC und Snapshot-Ersatz | T03 deterministische pre-guard Race-Fixtures, pending intent survives refusal |
| RHY/TIME/TMBR Feldschutz, PLOCK/MOTION/KEEP | T01-T04, T07; selektive Mutation und unveränderte Bank |
| Getrennte Keys, keine RNG-Kopplung | T01/T05; deterministische Contour-/Mutations-Fixtures |
| Ratchet bei negativem Microtiming | T06/T17; logische Step-ID und raw latch vor/nach Grenze |
| Live-Settings nach wartendem Commit | T02/T13/T16; POS/MOVE mit neuerem Wert, relevante Dependency stale |
| FM MODE/BRUT OFF/0, Tail-Latch | T08/T09; alte PCM und Voice-State, neue Klangcharaktere |
| SINE append, f(0)=0, Fade/Descriptor | T10/T11; graph/DSP/old save IDs, Legacy output |
| Morph POS/ON/Endpunkte und echter Hit-Roundtrip | T12-T14/T16; distinct endpoints and independent actual `v->x[0]` |
| Mode-aware MOVE und discrete CRUSH/CHORD | T15; 0/100/200 plus phase/RNG and effective type |
| Konsistenter Export inkl. live Änderungen | T14/T16; token in guarded chunk, final END, stale abort |
| UI scope, held-step priority, host/sim harness | T04/T07/T09/T11/T13/T16 |
| Memory/Stack/Audio evidence, frozen paths | BE00, each PR, T17; no new alpha gate |
| DNA-Kompatibilitaet, Lizenz, Editor IDs | T01-T03/T08/T10/T14/T17 |

**Rollback je PR:** bei nicht akzeptabler Regression den einzelnen Feature-PR nicht integrieren bzw. nach separater Freigabe gezielt revertieren, nie fremde Historie resetten. Native storage unverändert außer append SINE type; vor Downgrade Projekte mit neuem SINE explizit auf alten Typ setzen und sichern, da alte Firmware neue Sounds nicht verstehen muss. Native session recipe ist nicht automatisch persistent; Host-Datei bleibt beim Revert erhalten, alte Firmware kennt neue Capability nicht. Keine automatische Flash-Wiederherstellung oder Device-Writes. Bestehende Hardware-Alpha bleibt eigenstaendig.

**Pro PR Nachweisformat:** Base SHA, Head SHA, Scope-Diff, RED-Assertion, GREEN-Befund, alte goldens, Host/WASM/Native getrennt, Ressourcen vorher/nachher, Hardware status, offene Risiken. GitHub mergeable ist kein Ersatz für fachliche Prüfung. Ein Review muss auf den tatsächlichen PR-Head zeigen, nicht nur den alten Spezifikationscommit. Bei neuem main die betroffenen Quellen erneut prüfen und die Ressourcenrechnung aktualisieren.

**Self-review dieses Plans:** Abdeckung der sechs Features und neun Review-Risiken oben; vorgeschlagene Typen, Suite-Namen und APIs sind geplante Schnittstellen, nicht Behauptung vorhandener Implementierung. Keine Firmwareänderung, kein neuer PR, kein Merge oder Hardwaretest bei Planerstellung. Die genaue Zielgröße und Laufzeit werden erst durch Implementierungs- und Geraetemessung belegt. Revision 2: die sieben vorliegenden BE00-Rohdateien wurden unabhängig abgeglichen; keine erneute Kompilierung oder Hashbildung der nicht vorliegenden Firmware-/ELF-Binärdateien. Die portable Beispielimplementierung und Testfragmente sind Ausführungsentwürfe, keine bereits ausgeführten Firmwaretests. Die musikalischen v1-Kennlinien, Session-Schema und Nicht-Step-Hit-Semantik sind explizite Planentscheidungen. Die Draht-ID-Reservierungen sind gegen den tatsächlichen Implementierungs-Head erneut abzugleichen. Der Plan liefert konkrete Testvertraege und Integrationsschritte, keinen bereits kompilierten Gesamtpatch.


## 11. Superpowers-Ausführung: BE01 als erste eigenständige Implementierung

Dieser Abschnitt konkretisiert T01–T04. Die anderen 13 Tasks bleiben vollständig in §§5–9 erhalten. Bei der Umsetzung `superpowers:using-git-worktrees` vor dem ersten Schreibzugriff und anschließend `superpowers:subagent-driven-development` oder `superpowers:executing-plans` verwenden. Nicht automatisch auf `main` implementieren. Der Plan selbst autorisiert keine Commits, Pushes, PR-Eröffnung oder Hardwarezugriffe; die folgenden Befehle sind für eine später ausdrücklich gestartete Umsetzung.

### T01a – Pure Layer-Funktion und unabhängige Tests

**Files:** `firmware/src/grind/be.h`, `firmware/src/grind/be_layers.c`, `firmware/src/grind/be_math.c`, `firmware/src/grind/amalgam.c`, `tests/grind/test_be_layers.c`, `tests/run_grind_tests.sh`. Typen in T01 bleiben verbindlich.

- [ ] Lege die API aus T01 an und registriere die Suite (case und all, §3.1). Zum Start einen neutralen Stub `be_merge_cell` schreiben, der den Ausgang unverändert zurückgibt. Dann folgenden vollständigen Fixture-Kern in `tests/grind/test_be_layers.c` hinzufügen; `main` gibt bei allen bestandenen Assertions 0 zurück. Ein eigener Test darf keine erwarteten Zellen mit dem zu prüfenden Merge berechnen.

```c
#include <assert.h>
#include <string.h>
#include "be.h"

static void eq(gm_cell a, gm_cell b) {
    assert(a.flags == b.flags);
    assert(a.cond == b.cond);
    assert(a.note == b.note);
    assert(a.micro == b.micro);
}

int main(void) {
    const gm_cell a = { GM_ON | GM_ACC | GM_TIE | (2u << GM_RAT_SHIFT), 7, 12, -25 };
    const gm_cell b = { GM_ON | GM_SLIDE, 3, -5, 9 };
    const gm_cell rhythm = { GM_ON | GM_SLIDE, 3, -5, -25 };
    const gm_cell timing = { GM_ON | GM_ACC | GM_TIE | (2u << GM_RAT_SHIFT), 7, 12, 9 };
    const gm_cell off = { 0, 7, 12, -25 };
    gm_cell v = {0}, bad = b;
    assert(be_merge_cell(a,b,BE_RHY,0,&v)==GM_OK); eq(v,rhythm);
    assert(be_merge_cell(a,b,BE_TIME,0,&v)==GM_OK); eq(v,timing);
    assert(be_merge_cell(a,b,BE_RHY|BE_TIME,0,&v)==GM_OK); eq(v,b);
    assert(be_merge_cell(a,b,BE_TMBR,0,&v)==GM_OK); eq(v,a);
    assert(be_merge_cell(a,b,0,0,&v)==GM_OK); eq(v,a);
    assert(be_merge_cell(off,b,BE_ALL,1,&v)==GM_OK); eq(v,off);
    v = a;
    bad.flags = 0x80;
    assert(be_merge_cell(a,bad,BE_RHY,0,&v)==GM_INVALID); eq(v,a);
    assert(be_merge_cell(a,b,8u,0,&v)==GM_INVALID); eq(v,a);
    assert(be_merge_cell(a,b,BE_ALL,0,0)==GM_INVALID);
    bad = b; bad.micro = 51;
    assert(be_merge_cell(a,bad,BE_TIME,0,&v)==GM_INVALID); eq(v,a);
    bad = b; bad.cond = 59;
    assert(be_merge_cell(a,bad,BE_RHY,0,&v)==GM_INVALID); eq(v,a);
    return 0;
}
```

- [ ] `sh tests/run_grind_tests.sh be_layers` – RED bei der ersten RHY-Ergebnisprüfung. Ein Kompilierfehler allein reicht nach Anlage des Stubs nicht. Prüfen, dass der Stub wirklich die geplante API bereitstellt.
- [ ] Ersetze den Stub durch die feldgenaue Funktion (gemeinsame Zellvalidierung aus `core.c` nur dann faktorisieren, wenn ohne Legacy-Änderung; ansonsten den folgenden klar begrenzten Helper mit identischen Grenzen verwenden und dessen Gleichheit im Test prüfen):

```c
#include "be.h"
static int be_cell_valid(gm_cell c) {
    return !(c.flags & ~GM_FLAGS_MASK) && c.cond <= 58u &&
        c.note >= -60 && c.note <= 67 && c.micro >= -50 && c.micro <= 50;
}
gm_result be_merge_cell(gm_cell base, gm_cell candidate, unsigned layers,
                        int protected_cell, gm_cell *out) {
    gm_cell next = base;
    if (!out || (layers & ~((unsigned)BE_ALL)) ||
        !be_cell_valid(base) || !be_cell_valid(candidate)) return GM_INVALID;
    if (!protected_cell) {
        if (layers & BE_RHY) {
            next.flags = candidate.flags;
            next.cond = candidate.cond;
            next.note = candidate.note;
        }
        if (layers & BE_TIME) next.micro = candidate.micro;
    }
    *out = next;
    return GM_OK;
}
```

- [ ] Ergänze die Key-/TIME-Fixtures in T01, z. B. identische `cfg/key/track/step` ergibt wieder denselben Micro-Wert; unabhängiger Referenzcode berechnet `span=min(12,600*bpm*division/240000)*humanize/100*(250-swing)/250` mit exakt derselben ganzzahligen Auswertungsreihenfolge wie `core.c`. `step==0` ergibt 0..span, andere -span..span. Gesperrte Ebene und alle anderen Tracks bleiben bei einer Mutation unabhängig; nicht als Test verlangen, dass zwei Zufallswerte immer verschieden sind (zulässige Kollisionen). Vergleiche deterministische Referenzvektoren über mehrere Schritte/Keys.
- [ ] `sh tests/run_grind_tests.sh be_layers`; `sh tests/run_grind_tests.sh golden`; `sh tests/run_grind_tests.sh properties`; `sh tests/run_grind_tests.sh locks`. Erwartung: Exit 0, alte Golden-Dateien unverändert. Portable WASM-Suite in CI ausführen; danach den begrenzten T01-Commit aus §11.5 erstellen.

### T02a – Bounded Native Stage, zwei unterschiedliche Revisionen

**Files:** `firmware/src/grind/runtime.c`, `firmware/src/grind/controller.h`, `firmware/src/grind/controller.c`, `firmware/src/grind/be_runtime.c`, `firmware/src/grind/seq_adapter.c`, `firmware/src/core.h`, `firmware/src/felucca.c`, `tests/drum_host.h`, `tests/grind/test_be_runtime.c`, `tests/run_grind_tests.sh`.

- [ ] Vorher-/Nachher-Snapshot im **Hosttest**, nicht in Firmware: `step_t before[8][64]`, `uint8_t timing[8][4]`, Kopie von KEEP und Lockbank. Setze auf Track 2 bei Länge 16 die native Zelle 48 auf einen eindeutig nichtleeren Zustand (on=1, acc=1, nt=7, micro=-9, rat=2); verwende die tatsächlichen `step_t`-Felder und zuerst gültige native Timingwerte. Wähle ein vom Generator unterstütztes Style-/Raster-Fixture mit mindestens einem tatsächlich veränderten spielbaren Step. Negative Kontrolle: nach LEN64 ist die Zelle hör-/lesbar vorhanden. Alle Tests mit NSTEP=64, nicht nur Länge 16.
- [ ] Ergänze bei 16/12/15-Tracks den echten cue-Test aus `tests/grind/test_cue.c`: Ein BE-Submit am laufenden Transport erzeugt noch keinen Commit. Die Prüfgröße ist `gm_runtime_commits()` plus Vergleich der tatsächlich veränderten aktiven Zelle/Key, nicht allein Status. Vor END bytegleich, an END genau ein Commit. Weitere Audiohalfpuffer dürfen nicht erneut committen. Stopped-Variante muss sofort nach erfolgreichem Poll übernehmen. `GM_BOUNDARY_BAR` und laufendes SONG verweigern.
- [ ] Teste superseded WRITING/READY, Native-Cue-Vorrang und STOP über die vorhandene Controller-/Cue-Logik. Der physische Worker-Owner bleibt bis zum Rücksprung reserviert, auch wenn `gm_ctl_cancel` den logischen Status FREE setzt. Setze die Hook genau zwischen zwei geschützten Copy-Slices; ein echter nativer Edit erhöht die Basisrevision, der Candidate wird verworfen. Keine Wiederverwendung der Union bis zum tatsächlichen Worker-Ende.
- [ ] `sh tests/run_grind_tests.sh be_runtime` – RED auf den nicht erhaltenen oder nicht geänderten Testzustand. Die neue Test-Suite in `case` und `all` registrieren. Zunächst keine produktive Mutation freigeben, wenn nur die Schnittstelle steht.
- [ ] Implementiere die konkreten Übergänge nach T02: aktives Request/Scope/Context-Ticket innerhalb Guard erfassen; neue Request-Kind-ID append-only; `be_pending` atomar an Request-ID binden; `gm_ctl_begin` übergibt zugehörigen Payload an `be_stage` und markiert physischen Owner. Nur Main-Loop generiert. Jede Source-Slice prüft dieselbe erwartete Basisrevision und denselben Kontext. Der portable Kandidat wird außerhalb des Guard validiert; bei stale Input früh ablehnen. Wenn der Core-Base nicht mehr gebraucht wird, Union auf Stage umstellen; vollständige native Daten mit derselben Ticket-Prüfung in kleinen bounded Slices lesen, dann ausschließlich freigegebene Feldänderungen schreiben. Bei TMBR-only kein Patternexport/Stage-Copy. An READY `gm_ctl_finish` erst nach Abgleich des Payload-Tickets. An Consume `gm_ctl_consume` und Schreibset/Abhängigkeitsprüfung in derselben Guard wie Commit. Für RHY/TIME die komplette bereits erhaltene native Stage verwenden; niemals `gm_to_native` auf alle 512 Zellen anwenden. Die native Commit-Historie einmal erfassen, Metadaten zum Zeitpunkt des Commits übernehmen, nicht zum Zeitpunkt des Submit. Kein Copy von POS/ON/MOVE oder BRUT, wenn nicht im Write-Set. Kein READY ohne Änderung. Physisches Owner-Flag bei jeder Worker-Rückkehr freigeben; die lesende Stage selbst bleibt bis RETIRED/reconcile geschützt.
- [ ] Teste eine echte Änderung an einer nicht gesperrten Rhythmus-/Timing-Zelle und danach alle unbeteiligten Daten per `memcmp` gegen die vorher gesicherte Fixture. Verändere im Test gezielt den Commit-Pfad zu einer Nullung dormanter Steps: der Test muss scheitern. Entferne den Mutanten vor dem Commit. Gegenprobe KEEP außerhalb LEN darf nicht durch automatisches Löschen der KEEP-Maske scheinbar gelöst werden.
- [ ] Green: `be_runtime`, `adapter`, `controller`, `cue`, `golden` einzeln laufen lassen; `git diff --check` und native Größenprobe von `be_recipe`, `be_patch`, zusätzlichem statischem State. Ziel 448 B für BE01 einschließlich späterem T03/T04, nicht nur `sizeof(be_recipe)`.

### T03a – Undo als atomarer, identitätsgebundener Tausch

**Files:** `firmware/src/grind/runtime.c`, `firmware/src/grind/be_runtime.c`, `firmware/src/core.h`, tatsächliche Schreibstellen laut T03; `tests/grind/test_be_runtime.c`, `tests/grind/test_edit_sites.py`.

- [ ] Lege den Host-only Pre-Guard-Hook an genau der in T03 bezeichneten Stelle an; im Target wird er vollständig herauskompiliert. Testfall A: tatsächlichen `rec_hit`-Pfad nach der Beobachtung des Undo-Snapshots, aber vor der entscheidenden Guard auslösen; assert, dass der Step zuvor leer, danach belegt und verschieden ist. Testfall B: tatsächlichen Commit B in diesem Fenster auslösen, gültigen Ersatzsnapshot und danach Request C erzeugen; eine nur auf `undo_ok` prüfende Implementierung würde fälschlich B rückgängig machen. Testfall C: erfolgreicher Undo, direkt anschließender echter REC-Hit invalidiert History und bleibt bestehen. Testfall D: ohne Interleaving zwei erfolgreiche Aufrufe, exakte alte/neue Steps, Timing, Rezept und aktive Konfiguration. Testfall E: `song`-Zustand wechselt im Fenster, Refusal ohne Pending-Abbruch durch Undo. Die Fixture prüft die Revision **nach** dem konkurrierenden Edit, damit ein korrekt invalidierender REC-Pfad nicht als falscher zusätzlicher Undo-Effekt gewertet wird.
- [ ] `sh tests/run_grind_tests.sh be_runtime` – RED mindestens in A/B mit bisherigem Vorab-Check. Verwende die tatsächlichen Eingabewege und unabhängige Vorherbilder; keine Testsimulation, die nur `undo_ok=0` setzt und den echten Edit-Hook auslässt.
- [ ] Ersetze die Prüfung und den Guard wie in T03. `be_undo_identity_isr()` liefert den eindeutigen Snapshot-Zähler; `be_undo_matches_context_isr()` vergleicht den Snapshot-Kontext und die erwartete aktive Basisrevision. Beide sind unter bestehender IRQ-Sperre zu verwenden. Erfolgreicher Swap aktualisiert die erwartete Basisrevision und erhält Identität und Richtung als eine Undo/Redo-Historie; neuer geänderter Commit erzeugt neue Identität. Beim Edit die nativen Daten **und** Revision/History im selben geschützten Übergang ändern. Keine verschachtelten IRQ-off/on-Paare. Refusal ist vor `gm_runtime_stop` und allen Tausch-/Ack-/Dirty-Änderungen. Bei reinem Metadaten-Undo keinen nativen Step kopieren. Live-Parameter außerhalb des Write-Sets werden nicht zurückgedreht.
- [ ] Führe ein Edit-Site-Inventar im Arbeits-Checkout aus: `rg -n 'gm_touch(_isr)?\(|gm_context_isr\(|motion_add\(|motion_capture\(|lock_(set|clear)|rec_hit\(|keys_rec\(' firmware/src`; `python3 -m unittest discover -s tests/grind -p 'test_edit_sites.py' -v`. Verfolge jeden Treffer bis zu seinem tatsächlichen Writer. Die bestehenden Vorab-`gm_touch()`-Aufrufe müssen für betroffene Main-Schreibvorgänge mit dem effektiven Write eine gemeinsame Guard bilden, nicht bloß die alte Prüfung zeitlich verschieben. Bestehende Audit-Allowlist nur mit konkret begründeten sicheren Pfaden ergänzen. Keine pauschale Ausnahme für alle neuen BE-Dateien.
- [ ] RED-Mutanten: Gültigkeit nur vor Guard; Identität ignorieren; Snapshot bei eigenem Undo löschen; Pending vor der Refusal-Prüfung löschen. Jeder Mutant muss mindestens eine der A–E-Assertions verletzen. Original wiederherstellen. Green `be_runtime`, `cue`, `persist`, `ui`, `editor` einzeln; ASan/UBSan `be_runtime`; Edit-Site-Audit.

### T04a – UI als echter Firmwarepfad

**Files:** `firmware/src/grind/ui.c`, `firmware/src/pages.c`, `firmware/src/ui.c`, `firmware/src/ui_draw.c`, `firmware/src/ui_input.c`, `tests/ui_host.h`, `tests/sim_host.h`, `tests/grind/test_be_ui.c`, `tests/run_grind_tests.sh`.

- [ ] Erzeuge einen echten `ui_host.h`-Test analog `tests/grind/test_ui.c`, keine reine Descriptor-Prüfung. Setup: ausgewählte Spur, tatsächliche ARP-/LOCKS-Navigation und Encoderdrehung; nach `press` den nächsten `ui_frame` ausführen; HOME/REC-Tap erst nach Release plus Frame prüfen. Speichere die Lockmasken beider Tracks vorher; die Änderung muss ausschließlich auf die gewählte Spur gehen. Bei gehaltenem Step darf die Aktion MUT nicht ausgeführt werden. Trackwechsel muss aktuelle Proxies nachziehen; die Proxies sind keine persistente Quelle. TMBR bis BE02 als nicht verfügbar markieren. Links-/Rechtsdetent entsprechend T04, Action-Knob nach Aktion wieder neutral. `GRIND_NATIVE=0` keine neue Seite/IDs. Ein fake `chain.running` nicht während echter Frames laufen lassen.
- [ ] `sh tests/run_grind_tests.sh be_ui` – RED auf tatsächliche Navigation/Lockmasken/Aktion; Test muss bei nur kosmetisch dargestellter Seite ohne Mutation fehlschlagen.
- [ ] Implementiere die vorhandenen Seiten-/Descriptor-Erweiterungen gemäß T04; `GU_N`, `gm_v` und Bounds der alten Editor-Knobs konsistent halten. Falls neuer BE-Scope verwendet wird, `page_desc`, input routing und Host-/Sim-Glue gemeinsam integrieren. Legacy-IDs bleiben erhalten, keine neue Flash-Serialisierung. Dieselbe `be_submit`-API aus T02 aufrufen; UI liest Ack und echten Zustand, zeigt `PENDING END`, `APPLIED`, `LOCKED`, `STALE`, `NO CHANGE`, `LOCK CONFLICT` entsprechend Result. Bei STOP und Context-Reset kein falsches APPLIED. Vorschau benutzt vorhandene kompakte Daten; keine zweite Bitmap für alle Seiten.
- [ ] Green `be_ui`, `ui`, `menu`, `cue`, `boot_anim`, `boot_draw`, `boot_ui`, `boot_recovery`, `boot_restore`. Beide GRIND-Varianten upstream und Simulator prüfen. Screenshot-Nachweis aus echtem Host-/Simulator-Render, nicht aus einem Mockup. RAM-Ziel der gesamten BE01-Phase gegen BE00 vergleichen; vor PR die vorhandenen CI-Lanes und den Review aus §10 ausführen.

### 11.5 Ausführungsregister aller 17 Tasks

Die folgenden Einträge sind der kompakte Befehlsindex zu den vollständigen Aufgaben oben. Jede neue Suite wird **vor** RED in `tests/run_grind_tests.sh` (case + all) eingetragen. Native Suites mit `run_native`; rein portable `be_layers`, `be_timbre`, `be_morph` mit `run_test` und den benötigten `.c`-Dateien. Wenn eine Suite in einem früheren PR noch nicht vorhanden ist (z. B. `be_sine` bei unabhängiger BE04), nur die explizit gelandete Suite aufrufen und im Nachweis als noch nicht integriert ausweisen. Die Commit-Namen stammen aus T01–T17. Kein `git add -A`; nur Dateien aus der jeweiligen Task explizit stagen, vor Commit `git diff --cached --stat` und `git diff --cached --check`.

**T01**

- [ ] RED: `sh tests/run_grind_tests.sh be_layers`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_layers`, `sh tests/run_grind_tests.sh locks`, `sh tests/run_grind_tests.sh properties`, `sh tests/run_grind_tests.sh golden`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): define independent layer mutation contracts"`.

**T02**

- [ ] RED: `sh tests/run_grind_tests.sh be_runtime`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_runtime`, `sh tests/run_grind_tests.sh adapter`, `sh tests/run_grind_tests.sh controller`, `sh tests/run_grind_tests.sh cue`, `sh tests/run_grind_tests.sh golden`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): publish scoped native layer candidates"`.

**T03**

- [ ] RED: `sh tests/run_grind_tests.sh be_runtime`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_runtime`, `sh tests/run_grind_tests.sh cue`, `sh tests/run_grind_tests.sh persist`, `sh tests/run_grind_tests.sh ui`, `sh tests/run_grind_tests.sh editor`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "fix(grind): make scoped undo and edit invalidation atomic"`.

**T04**

- [ ] RED: `sh tests/run_grind_tests.sh be_ui`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh ui`, `sh tests/run_grind_tests.sh menu`, `sh tests/run_grind_tests.sh cue`, `sh tests/run_grind_tests.sh boot_ui`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): expose per-track layer locks and scoped mutation"`.

**T05**

- [ ] RED: `sh tests/run_grind_tests.sh be_timbre`, `sh tests/run_grind_tests.sh be_runtime`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_timbre`, `sh tests/run_grind_tests.sh be_runtime`, `sh tests/run_grind_tests.sh be_layers`, `sh tests/run_grind_tests.sh golden`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): add deterministic per-track timbre recipes"`.

**T06**

- [ ] RED: `sh tests/run_grind_tests.sh be_sound`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_sound`, `sh tests/run_grind_tests.sh cue`, `sh tests/run_grind_tests.sh vary`, `sh tests/run_grind_tests.sh hit_variation`, `sh tests/run_grind_tests.sh golden`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): resolve deterministic timbre at the actual FM hit"`.

**T07**

- [ ] RED: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh be_runtime`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh be_runtime`, `sh tests/run_grind_tests.sh be_sound`, `sh tests/run_grind_tests.sh ui`, `sh tests/run_grind_tests.sh menu`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): add timbre page and nondestructive mutation workflow"`.

**T08**

- [ ] RED: `sh tests/run_grind_tests.sh be_perc`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_perc`, `sh tests/run_grind_tests.sh be_sound`, `sh tests/run_grind_tests.sh golden`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(dsp): add latched bounded brutal FM percussion modes"`.

**T09**

- [ ] RED: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh be_perc`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh be_perc`, `sh tests/run_grind_tests.sh be_sound`, `sh tests/run_grind_tests.sh persist`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(ui): add per-track brutal percussion controls"`.

**T10**

- [ ] RED: `sh tests/run_grind_tests.sh be_sine`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_sine`, `sh tests/run_grind_tests.sh master_fx`, `sh tests/run_grind_tests.sh golden`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(dsp): add zero-centred sine insert without new effect buffers"`.

**T11**

- [ ] RED: `sh tests/run_grind_tests.sh be_sine`, `sh tests/run_grind_tests.sh persist`, `sh tests/run_grind_tests.sh editor`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_sine`, `sh tests/run_grind_tests.sh persist`, `sh tests/run_grind_tests.sh editor`, `sh tests/run_grind_tests.sh ui`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): expose and roundtrip the appended sine insert type"`.

**T12**

- [ ] RED: `sh tests/run_grind_tests.sh be_morph`, `sh tests/run_grind_tests.sh be_sound`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_morph`, `sh tests/run_grind_tests.sh be_sound`, `sh tests/run_grind_tests.sh be_perc`, `sh tests/run_grind_tests.sh vary`, `sh tests/run_grind_tests.sh golden`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): add compatible hit-latched FM morphing"`.

**T13**

- [ ] RED: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh be_runtime`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh be_runtime`, `sh tests/run_grind_tests.sh be_morph`, `sh tests/run_grind_tests.sh be_sound`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(ui): expose morph and preserve live controls across staged commits"`.

**T14**

- [ ] RED: `sh tests/run_grind_tests.sh be_session`, `sh tests/run_grind_tests.sh editor`; zusätzlich `node web/editor/test_be_session.mjs`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_session`, `sh tests/run_grind_tests.sh be_morph`, `sh tests/run_grind_tests.sh be_runtime`, `sh tests/run_grind_tests.sh editor`, `sh tests/run_grind_tests.sh persist`; zusätzlich `node web/editor/test_be_session.mjs` und `node web/editor/test_editor.mjs`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(editor): add revision-bound brutal evolution session transport"`.

**T15**

- [ ] RED: `sh tests/run_grind_tests.sh be_move`, `sh tests/run_grind_tests.sh be_sound`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_move`, `sh tests/run_grind_tests.sh be_sound`, `sh tests/run_grind_tests.sh be_morph`, `sh tests/run_grind_tests.sh master_fx`, `sh tests/run_grind_tests.sh vary`, `sh tests/run_grind_tests.sh hit_variation`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(grind): scale approved modulation contributions with movement"`.

**T16**

- [ ] RED: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh be_session`, `sh tests/run_grind_tests.sh be_move`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh be_ui`, `sh tests/run_grind_tests.sh be_session`, `sh tests/run_grind_tests.sh be_move`, `sh tests/run_grind_tests.sh editor`, `sh tests/run_grind_tests.sh persist`; zusätzlich `node web/editor/test_be_session.mjs` und `node web/editor/test_editor.mjs`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "feat(ui): integrate live movement and session controls"`.

**T17**

- [ ] RED: `sh tests/run_grind_tests.sh be_runtime`, `sh tests/run_grind_tests.sh be_sound`, `sh tests/run_grind_tests.sh be_session`, `sh tests/run_grind_tests.sh be_move`; neue fachliche Assertion muss fehlschlagen.
- [ ] Minimalen Task-Patch nach obigem Dateiplan implementieren, Scope prüfen.
- [ ] GREEN: `sh tests/run_grind_tests.sh all`; zusätzlich `node web/editor/test_be_session.mjs` und `node web/editor/test_editor.mjs`; Exit 0, unabhängige Sollwerte.
- [ ] Nach bestandenem `git diff --check` und explizitem Staging: `git commit -m "test(grind): verify brutal evolution integration and resource evidence"`.

### 11.6 Phasenabschluss und Ausführungsübergabe

- [x] BE00 statische Basis: Rohdatenabgleich gemäß §2.1a, CI am identischen Main-Commit PASS.
- [ ] BE01 T01–T04: eigenständiger PR mit realem Commit-/Undo-/REC-/UI-Pfad, kein DSP/Session-Protokoll in diesem PR. PR muss auf aktuelles `main` zeigen.
- [ ] BE02 T05–T07: eigenständiger PR; TIMBRE-only, realer FM-Trigger, Ratchet-Latch, native bytegenaue Erhaltung.
- [ ] BE03 T08–T09: eigenständiger PR; vier latched FM-Modi; OFF/0 exact Legacy.
- [ ] BE04 T10–T11: unabhängiger PR gegen aktuelles `main`, vor Integration mit weiteren PRs erneut messen; kein alter Insert-ID-Wechsel.
- [ ] BE05 T12–T14: eigenständiger PR; Morph und expliziter revisionsgebundener Session-Transfer, Wire-ID-Audit.
- [ ] BE06 T15–T17: eigenständiger PR; MOVE, vollständige Integration und End-to-End-Nachweise.
- [ ] Hardware-Nachweise BE00.2 bei verfügbarer und autorisierter FM-1-Hardware; keine Hardwarefreigabe auf Basis statischer CI.

**Vor jedem Übergang**: tatsächlichen `HEAD`, baseRefName, Diff und bestehende CI prüfen, nicht den hier eingefrorenen SHA als Implementierungsstand annehmen. Wenn eine geplante API mit dem neuen Head kollidiert, nur die abhängige Task neu planen. Keine zusätzlichen globalen Freigabekriterien und keine Verzögerung der bestehenden Hardware-Alpha. Alle sechs Funktionen behalten ihren Umfang; die Reihenfolge ist die Integrationsentscheidung dieses Plans.
