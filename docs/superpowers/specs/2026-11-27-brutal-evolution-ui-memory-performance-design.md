# BRUTAL EVOLUTION
## UI & Memory & Performance Architecture

**Architekturentwurf v0.1 · 27. November 2026**  
**Ziel:** GRIND MACHINE auf dem FM-1, zunächst GRIND_NATIVE=1 und FELUCCA_BLE=0.  
**Referenzen:** GRIND MACHINE `7b45e8197c45e5c718d670ffa42481724811c76e`; BRUTAL FM `d2081886152cc7fb716cdeb476ee56e9e4437cc0`.

Dieser Entwurf verbindet die sechs zuvor priorisierten Erweiterungen: LAYER LOCKS, TIMBRE DNA, BRUTAL PERC, SOUND MORPH, SINE FOLDER und MOVEMENT. Er legt Bedienung, Zustandsbesitz, Ressourcenrechnung und Integrationsreihenfolge fest. Quellcode und vorhandene Dokumentation wurden gelesen. Es wurden keine Firmwaredateien verändert, keine neuen Builds oder Hardwaretests ausgeführt und keine Änderungen veröffentlicht.

**Kennzeichnung:** „Quellbefund“ beschreibt den untersuchten Stand. „Entwurfsentscheidung“ beschreibt das vorgeschlagene Verhalten. Zahlen aus Beispielrechnungen sind keine Messwerte einer implementierten Erweiterung.

## 1. Architekturentscheidung

BRUTAL EVOLUTION erweitert die vorhandene GRIND-Engine an ihren bestehenden Übergabepunkten: Rhythmus und Timing über den Generator und seine Stage, Klang über die Parameterauflösung und die FM-Stimme, Verzerrung über INSERT, Modulation über den vorhandenen LFO-Pfad. Sequencer, Audioausgabe, Undo-Puffer und Bildschirm-Canvas bleiben die gemeinsamen Ressourcen.

Die erste Sound-Integration unterstützt das generische Zweoperator-FM-Modell ausdrücklich. Weitere Modelle erhalten Unterstützung erst mit einer überprüften Parametertabelle. Ein vollständiger Vieroperator-Port, eine zweite Audioengine, globale Überabtastung und zusätzliche Delay-Puffer gehören nicht zu diesem Umfang.

| Erweiterung | Entscheidung für die erste Ausbaustufe | Wichtigste Grenze |
| --- | --- | --- |
| LAYER LOCKS | Drei unabhängige Mutationssperren je Track | Sperren schützen vor Generierung, nicht vor bewusster Live-Bedienung |
| TIMBRE DNA | Kompaktes, reproduzierbares Klangrezept je Track | Keine vollständige Parametermatrix pro Step |
| BRUTAL PERC | Ein gewählter Modus und eine Intensität im vorhandenen FM-Modell | Bestehende acht FM-Parameter behalten ihre Bedeutung |
| SOUND MORPH | Zwei kompatible Parametersätze; neue Werte beim nächsten Hit | Ausklingende FM-Stimmen ändern sich zunächst nicht nachträglich |
| SINE FOLDER | Ein zusätzlicher INSERT-Typ mit bestehendem Effektzustand | Der Folder bearbeitet den gesamten Track |
| MOVEMENT | Globaler Faktor für ausdrücklich ausgewählte Klangmodulationen | Timing, Gate, Velocity, VARY, SINK und SWELL bleiben außerhalb |

**Quellbefund:** GRIND besitzt bereits eine Zweoperator-FM-Stimme, einen Insert-Wavefolder und ein LFO-System. Die Runtime generiert im Hauptprogramm und veröffentlicht einen geprüften Kandidaten am nativen Cue-Übergang. Diese vorhandenen Strukturen bilden die Integrationspunkte. [FM][fm] · [INSERT][insert] · [LFO][lfo] · [Runtime][runtime]

Die bestehende DNA-Spezifikation beschreibt außerdem BREED, BRAIN und INPUT. Das sind geplante Erweiterungen. BRUTAL EVOLUTION nutzt deren gemeinsamen Vertrag für Revisionen, Schutzregeln und Metadaten; RIFF-Capture und DRUMMER BRAIN müssen für einfache Klangmutation nicht zuerst vollständig umgesetzt werden. Eine im älteren DNA-Dokument erwähnte ungültige Reset-Konfiguration ist im hier untersuchten Code bereits auf die gültige Factory-ID 1 korrigiert. [DNA-Entwurf][dna] · [Runtime-Reset][runtime-reset]

## 2. UI: vorhandene Bedienfamilien erweitern

### 2.1 Navigation und Seiten

**Quellbefund:** Die Sektionstaste öffnet die zuletzt verwendete Seite ihrer Familie; wiederholtes Drücken blättert weiter. SELECT navigiert innerhalb der Familie, ALGORITHM wählt den Track. Die vier Parameterregler sind Drehregler. OCT+ öffnet aus SOUND die vorhandene Sample-Seite LAYER; OCT− führt zurück. [Panel][panel] · [Seitennavigation][navigation] · [SOUND/LAYER][pages]

**Entwurfsentscheidung:** Die vorhandene Reihenfolge bleibt erhalten. Neue Seiten werden an die passende Familie angehängt. Das Wort LAYER bleibt für die Sample-Schicht reserviert; die neuen Mutationssperren heißen auf dem Gerät LOCKS. MOTION bleibt der Name der aufgezeichneten Automation; der neue Live-Regler heißt MOVE, ausgeschrieben MOVEMENT.

| Zugang / Seite | Geltung | Regler 1 | Regler 2 | Regler 3 | Regler 4 |
| --- | --- | --- | --- | --- | --- |
| ARP → bestehende LIVE-Seite | Alle Tracks | SINK | SWELL | CUT | **MOVE** |
| ARP → neue LOCKS-Seite | Gewählter Track | RHY | TIME | TMBR | MUT |
| EDIT → bestehende SOUND-Seiten | Gewählter Track | Bisherige Belegung | Bisherige Belegung | Bisherige Belegung | Bisherige Belegung |
| EDIT → BRUTAL | Generisches FM-Modell | MODE | BRUT | TUNE | DEC |
| EDIT → TIMBRE | Kompatibler Track | CURV | AMT | SPAN | TARG |
| EDIT → MORPH | Kompatibler Track | POS | CAP A | CAP B | ON |
| FX → bestehende INSERT-Seite | Gewählter Track | TYPE: SINE | FOLD | SYMM | BIAS |
| FX → zweite INSERT-Seite | Gewählter Track | MIX | SRR | — | — |

BRUTAL bietet OFF, METAL, GROWL, CHAOS und NOISE als alternative Modi. BRUT bestimmt ihre Intensität. TUNE und DEC verwenden die vorhandenen FM-Parameter; sie benötigen keine zweiten gespeicherten Werte. Die vier Klangcharaktere sind in dieser Stufe keine vier parallel laufenden Engines.

TIMBRE zeigt den vollständigen Zielnamen in Graph oder Fußzeile. Die erste FM-Zielliste besteht aus INDEX, MDEC und FBK. MORPH zeigt A/B jeweils als EMPTY oder READY sowie Modell und BRUTAL-Modus. Unvereinbare Endpunkte ergeben MODEL MISMATCH. Nicht unterstützte Klangmodelle zeigen eine eindeutige Nichtverfügbarkeit; die Bedienung ändert dabei keine versteckten Parameter. MORPH startet mit ON=OFF und lässt sich erst mit zwei gültigen, kompatiblen Endpunkten aktivieren.

**Quellbefund:** LIVE besitzt bereits SINK/SWELL/CUT und eine freie vierte Spalte. Die ARP-Familie enthält zwei GRIDS- und fünf GRIND-Seiten. Die Fußzeile zählt die ganze Familie. Deshalb werden keine dauerhaft gültigen Seitenzahlen versprochen; LOCKS und spätere DNA-Seiten verändern den Nenner. [Seitentabelle][pages] · [Fußzeile][footer]

### 2.2 Aktionen und Eingabepriorität

CAP A und CAP B werden als benannte Drehaktionen umgesetzt: ein Rechtsdetent erfasst den Basis-Sound, anschließend kehrt das Aktionsfeld in seinen Ruhezustand zurück. Das folgt vorhandenen APPLY-/KIT-Konventionen. Es wird keine Encoder-Druckfunktion vorausgesetzt. Ein vorhandener Endpunkt wird durch die bewusst ausgelöste CAP-Aktion ersetzt; die neue Erfassung wird sichtbar bestätigt. [Vorhandene Aktionen][actions]

Ein gehaltener Step behält Vorrang. Wenn ein Regler für PLOCK nicht unterstützt wird, löst die Bewegung keine darunterliegende CAP- oder Mutationsaktion aus. Der vorhandene lange Step-Hold auf GRIND-Seiten behält seine KEEP-Funktion; er wird nicht gleichzeitig als Layer-Lock-Geste verwendet. [Eingabepriorität][input-priority] · [KEEP-Geste][keep-gesture]

Die LOCKS-Seite wirkt zunächst auf den ausgewählten Track. Gemeinsames Setzen aller Tracks kann später als ausdrücklich benannte Aktion ergänzt werden. Ein unsichtbarer globaler Zustand hinter drei lokal aussehenden Schaltern wird vermieden.

MUT auf der LOCKS-Seite ist eine eigene benannte Drehaktion: ein Rechtsdetent bereitet eine Mutation aller dort entsperrten Ebenen des gewählten Tracks vor. Sind alle drei Ebenen gesperrt, erscheint LOCKED und es entsteht kein Auftrag. Für reine Klangmutation stehen RHY und TIME auf LOCK, TMBR auf FREE. Der bisherige GRIND-MUT-Regler behält außerhalb dieser neuen Aktion seine bisherige Bedeutung. Ein durch trackübergreifende musikalische Regeln unvereinbarer Rhythmuswunsch wird abgelehnt, statt ungefragt andere gesperrte Tracks zu ändern.

### 2.3 Anzeige und Rückmeldung

Die Kopfzeile nennt Seite und Geltung: beispielsweise „TIMBRE · T3“ oder „LIVE · ALL“. Der Graph zeigt eine kleine Klangkontur, den Morph-Punkt oder drei Sperrzustände. Der laufende Triggerstreifen bleibt sichtbar. Status wird mit Text und Form dargestellt, etwa PREVIEW, PENDING END, APPLIED, STALE, NO CHANGE oder LOCK CONFLICT. Gesperrt, wartend und abgelehnt sind unterschiedliche Zustände.

Pattern-Mutationen werden bei laufender Wiedergabe für den nativen END-Übergang vorbereitet; bei gestopptem Transport ist eine geprüfte Übernahme unmittelbar möglich. END bezeichnet den vorhandenen nativen Pattern-Abschluss, nicht einen neu eingeführten Taktzähler. Live-MOVE und die Morph-Position werden dagegen nach ihrem Klangvertrag am Block beziehungsweise nächsten Hit wirksam. Die UI darf diese Regler nicht pauschal als „wartet auf END“ darstellen. [Runtime][runtime] · [GRIND-UI][grind-ui]

### 2.4 Rendering ohne neuen Bildpuffer

**Quellbefund:** Der Bildschirm ist in Kopfzeile, vier Reglerfelder, Graph, Triggerstreifen und Fußzeile aufgeteilt. Die Graphfläche liegt bei y=61..180. Der Renderer verwendet einen gemeinsamen RGB565-Streifenpuffer von 240×124 Pixeln, also 59.520 Byte, in .pool. Wertefelder zeigen höchstens vier Glyphen; lange Werte erscheinen beim Drehen in der Fußzeile. [Layout][layout] · [Reglerfelder][knob-draw] · [Canvas][gfx]

Die neuen Seiten zeichnen in diesen vorhandenen Canvas. Konturen werden aus wenigen aktuellen Werten berechnet; es gibt keine eigenen Bitmap-Puffer je Seite. Statische DNA-Daten werden bei Parameter- oder Revisionsänderungen neu gezeichnet. Das folgt dem bereits verwendeten Verfahren für teilweise Neuzeichnung. Ein zusätzlicher vollständiger RGB565-Framebuffer würde rechnerisch 240×240×2 = 115.200 Byte benötigen. [Neuzeichnung][redraw]

## 3. Drei Mutationsebenen mit klarer Zuständigkeit

### 3.1 Was die Sperren schützen

| Ebene | Veränderbarer Inhalt | Wirkung der Sperre |
| --- | --- | --- |
| RHYTHM | Schlagbelegung, Bedingungen, Ratchets und zugehörige Step-Attribute im gewählten Generatorumfang | Diese Step-Felder werden aus der aktiven Basis übernommen |
| TIMING | Microtiming und, soweit ausdrücklich angeboten, Swing | Bestehende Zeitversätze bleiben erhalten |
| TIMBRE | Kontur, Ziel, Intensität, Schlüssel und BRUTAL-Einstellungen der Klangmutation | Das aktive Klangrezept bleibt erhalten |

Für die erste Layer-Mutation bleiben Länge, Division und Quelle unverändert. Änderungen des musikalischen Rasters gehören zu einer eigenen, vollständig validierten Generierungsaktion. Das verhindert, dass ein „Rhythmus erhalten“ scheinbar aktives Pattern durch eine andere Division dennoch anders spielt. Neue Timbre-Mutation schreibt keine Step-Noten, Accent-Werte oder Ratchet-Zahlen.

RHYTHM-Lock übernimmt die betroffenen nativen Step-Felder einschließlich Note/Slide/Tie, sofern solche Felder im Modell vorkommen. TIMING-Lock betrifft die Zeitkomponente. Die Zusammenführung muss den vollständigen bestehenden Step-Datensatz bewahren, nicht nur dessen ON-Bit.

KEEP bleibt der Schutz vollständiger bestehender Zellen. PLOCK und MOTION behalten ihre gespeicherten Orte und Werte. Die geplante DNA-Schicht behandelt deren Koordinaten bei einer Strukturmutation zusätzlich als temporären Schutz einschließlich erforderlichen Track-Timings; sie erzeugt daraus keine dauerhaften KEEP-Bits. Unvereinbare Schutzvorgaben werden als Konflikt gemeldet, bevor etwas live geht. [KEEP-Runtime][runtime-keep] · [DNA-Schutzvertrag][dna]

Layer-Locks schützen vor **Mutation**. Sie stoppen weder den Sequencer noch die Wiedergabe von Automation. Wer trotz TIMBRE-Lock bewusst MOVE oder MORPH bewegt, verändert den Klang weiterhin. Diese Unterscheidung steht auf der LOCKS-Seite als MUTATION ONLY.

### 3.2 Reproduzierbarkeit und unabhängige Zufallsentscheidungen

**Quellbefund:** GRIND besitzt bereits einen schlüsselbasierten Zufallszugriff. Das Ergebnis hängt von Seed, Mutation, Engine-Version, Algorithmus, Track, Step und Domain ab. Unterschiedliche Domains allein machen die Ebenen noch nicht unabhängig: ein gemeinsam veränderter Mutationszähler würde mehrere Ebenen beeinflussen. [RNG][rng]

**Entwurfsentscheidung:** Jeder Track erhält getrennte wirksame Schlüssel für Rhythmus, Timing und Timbre. Eine selektive Mutation verändert nur den Schlüssel und die Daten der gewählten, entsperrten Ebene. Der unveränderte Legacy-Pfad verwendet seine bisherigen Domains und Ergebnisse. Zusätzliche Domain-IDs werden mit den bereits geplanten DNA-Domains abgestimmt und anschließend stabil gehalten.

Die Default-Timbre-Kontur ist an logische Step-Positionen gebunden. Ein neuer Schlag an Step 9 erhält den für Step 9 definierten Klangwert; das Einfügen eines früheren Schlags verschiebt nicht alle späteren Klangwerte. Ratchets desselben Steps verwenden zunächst denselben Timbre-Wert. CHAOTIC ist ein deterministischer Verlauf aus Rezept und Position. Weder UI-Aufrufe noch Audio-Blockzahl verändern diesen Verlauf.

Mögliche Konturen sind ASCEND, DESCEND, ALTERNATE, CHAOTIC, MOTIF und PULSE. SPAN begrenzt die musikalische Länge des Verlaufs innerhalb des bestehenden Rasters. AMT=0 hebt die zusätzliche Auslenkung auf. Dieselben Rezepte, Basismuster und Versionen ergeben dieselben Ergebnisse; bloß dieselbe Seed-Zahl ohne den übrigen Kontext reicht nicht als Reproduktionsangabe.

## 4. Zustandsbesitz, Commit und Undo

~~~mermaid
flowchart TD
    UI["UI und Editor"] --> REQ["Unveränderlicher Auftrag"]
    REQ --> MAIN["Main Loop: erzeugen und prüfen"]
    MAIN --> STAGE["Vorhandene Stage mit Revision"]
    STAGE -->|"Native END oder gestoppt"| ACTIVE["Aktives Pattern und Klangrezept"]
    ACTIVE --> SEQ["Nativer Sequencer"]
    ACTIVE --> PARAM["Klangparameter auflösen"]
    UI -->|"Live MOVE und MORPH"| PARAM
    SEQ --> PARAM
    PARAM --> DSP["Bestehende Stimmen und Effekte"]
~~~

**Quellbefund:** Der Controller besitzt FREE/WRITING/READY/READING/RETIRED-Zustände, eine neueste wartende Anfrage, eine Stage und Revisionen gegen veraltete Ergebnisse. Ein nativer Slot- oder Variationswechsel hat Vorrang. Die Audioseite übernimmt vorbereitete Steps und Timingwerte mit begrenzten Kopien. [Controller][controller] · [Runtime][runtime]

Die neue Beschreibungsschicht folgt demselben Besitzmodell:

1. ACTIVE bleibt während der Generierung lesbar. Audio liest keinen gerade bearbeiteten Entwurf.
2. WRITING/STAGED besitzt ein unveränderliches Rezept mit Basis- und Auftragsrevision.
3. LATEST-PENDING darf eine neuere Bedienabsicht enthalten, ohne den Worker-Snapshot zu überschreiben.
4. UNDO/REDO hält genau einen vorherigen aktiven Zustand. Der nächste erfolgreiche, tatsächlich verändernde Publish ersetzt ihn; Slot-, Variations- oder Projektwechsel sowie zwischenzeitliche unabhängige manuelle Edits oder Schutzänderungen invalidieren ihn gemäß DNA-Vertrag. Dazu gehören native Step-/Timing-Edits, KEEP/Layer-Locks, relevante PLOCK-/MOTION-Edits sowie manuelle Änderungen an den vom Snapshot wiederhergestellten Rezept-, Basis- oder Performance-Daten. Native Undo-Daten und neue Undo-Metadaten werden gemeinsam mit dem Edit invalidiert; UNDO darf danach nicht mehr tauschen. Interne Revisionserhöhungen durch den Undo-/Redo-Tausch selbst dürfen dessen Snapshot nicht löschen. Gewöhnliches STOP, Ablehnung und NO CHANGE erhalten ihn. [DNA-Undo-Vertrag][dna]

**Atomare Undo-Gültigkeit:** `gm_runtime_undo` prüft heute `chain.running` und `undo_ok` vor `fm1_irq_off()`. `rec_hit` kann in diesem Fenster im Audio-ISR über `gm_touch_isr()` die Basis ändern und einen Step aufnehmen. Für die neue Undo-Semantik müssen die entscheidende Prüfung von Snapshot-Gültigkeit, zugehörigem aktivem Kontext/Revision (einschließlich etwaiger vorab gelesener Snapshot-Identität) und Song-Zustand, der Abbruch wartender Übernahmen sowie der Pattern-/Metadaten-Tausch in derselben IRQ-off-Guard stattfinden. Bei ungültigem oder inzwischen ersetzt/inkompatibel gewordenem Snapshot: IRQ wieder freigeben und ohne Tausch, ohne Richtungswechsel, ohne Revisions- oder Pending-Änderung durch die abgelehnte Undo-Aktion zurückkehren. Alle unabhängigen Schreibpfade auf die wiederherstellbaren Daten, ausdrücklich Live-REC einschließlich KEYS-Aufnahme und relevanter ISR-MOTION-Aufnahme, müssen Gültigkeit/Metadaten und Kandidatenrevision mit dem effektiven Edit unter derselben Synchronisation invalidieren; Main-Loop-Schreiber verwenden eine entsprechend geschützte Edit-Transaktion. Bereits im ISR beziehungsweise unter IRQ-off wird kein zweiter IRQ-off/on-Zyklus geöffnet. Die Revisionserhöhung des erfolgreichen Undo-/Redo-Tauschs selbst aktualisiert die passende aktive Snapshot-Zuordnung, ohne ihn als fremden Edit zu invalidieren. Keine neue Kopie des Patterns und keine längere Generierungsarbeit im Guard. [Aktuelles Commit/Undo][runtime-undo] · [Live-REC][live-rec]

Pattern, Timing, gültige Rezeptidentität und zugehörige Metadaten werden gemeinsam übernommen. Das vorhandene exakte Pattern-Undo wird um passende Metadaten erweitert. Es wird nicht durch bloßes erneutes Generieren aus einem Seed ersetzt: manuelle Edits und KEEP lassen sich dadurch nicht allgemein rekonstruieren. [Aktuelles Commit/Undo][runtime-undo]

**Native Erhaltung bei Layer-Mutation:** `gm_native_export` exportiert nur Steps innerhalb LEN und setzt die übrigen Core-Zellen auf null; `gm_runtime_commit` überschreibt dagegen sämtliche 64 nativen Steps je Track. Eine unveränderte Export-/Publish-Rundreise ist deshalb für die neue selektive Mutation unzulässig. TIMBRE-only veröffentlicht ausschließlich neue Klangmetadaten an der geprüften Commit-Grenze; alle nativen Steps (auch jenseits LEN), Timingwerte und anderen Tracks bleiben bytegenau unverändert. Für RHY/TIME-Mutationen wird nach Ende der Core-Base-Lebensdauer die vorhandene native Stage aus dem vollständigen nativen Ausgangszustand derselben Revision aufgebaut; nur ausdrücklich freigegebene Felder spielbarer Steps des gewählten Tracks und zulässige Timingfelder werden ersetzt. Dormante Steps, unbeteiligte Tracks, KEEP und PLOCK-/MOTION-Daten bleiben unverändert. Lässt sich ein Schutzkonflikt (auch KEEP jenseits LEN) nicht darstellen, wird abgelehnt, nicht stillschweigend geleert. Die Revisionsprüfung vor READY und Consume stellt sicher, dass die erhaltenen Daten noch zur aktiven Basis gehören. Kein zusätzlicher vollständiger Pattern-Puffer; der bisherige Legacy-Generierungspfad bleibt unverändert. [Native Export][native-export] · [Native Commit][native-commit]

Eine Änderung von KEEP, Layer-Locks, relevanten PLOCK-/MOTION-Daten oder des Basiskontexts invalidiert betroffene Kandidaten und Previews. Ein vor dem Setzen einer Sperre berechneter Kandidat darf danach nicht unbemerkt veröffentlicht werden. Vor READY und vor Übernahme wird die Revision geprüft.

STOP verwirft eine wartende Übernahme. Ablehnung und NO CHANGE ersetzen weder das aktive Rezept noch Undo. Preview ist lesend und erhöht keinen aktiven musikalischen Mutationsstand. Request-IDs und musikalische Schlüssel sind getrennte Begriffe: schnelle neue Anfragen können ältere ersetzen, ohne eine fehlgeschlagene Anfrage als gespielte Mutation zu verbuchen.

Die bestehende Vorschau speichert bereits kompakte ON/ACC/RAT-Bitmaps für alle 64 Steps. Sie nutzt den gemeinsamen Generierungsbereich nur, wenn Stage und Pending frei sind; andernfalls meldet sie BUSY. BRUTAL EVOLUTION erweitert diese Vorschau, statt einen weiteren vollständigen Pattern-Puffer anzulegen. [Vorschau][preview]

## 5. Klangarchitektur und Parameterauflösung

### 5.1 Bestehende Automation bewahren

**Quellbefund:** Der derzeitige Klangpfad folgt Basiswert → MOTION → Step-PLOCK → LFO-Auslenkung um den so bestimmten Mittelpunkt. PLOCK funktioniert auch bei ausgeschaltetem MOTION PLAY; MOTION CLEAR entfernt PLOCK nicht. Der Zugriff motion_rest liefert den ruhenden Basiswert ohne vorübergehende Lock-, Motion- oder LFO-Werte. [Motion-Vertrag][motion] · [Basiswert-Zugriff][motion-rest]

**Entwurfsentscheidung:** Die Reihenfolge lautet: MORPH-Ergebnis beziehungsweise bisheriger Basiswert → mit MOVE skalierte DNA-Auslenkung → Begrenzung auf den Parameterbereich → MOTION → PLOCK → mit MOVE skalierte zugelassene LFO-Auslenkung → Zielbegrenzung. MORPH und TIMBRE DNA bilden somit eine zusätzliche, flüchtige Basis für unterstützte Klangparameter. MOTION und PLOCK behalten ihre bisherige Vorrangstellung. Neue Beiträge schreiben weder dauerhaft in die Parameterbank noch in die Lockbank. Bei AMT=0 und deaktiviertem MORPH entspricht die Basis dem bisherigen Wert.

CAP A/B erfasst die ruhenden Basisparameter sowie den gewählten BRUTAL-Modus und seine Basisintensität, ohne MORPH-, DNA-, MOTION-, PLOCK- oder LFO-Beitrag. Es zeichnet keinen vorübergehend modulierten Momentanwert auf. Eine spätere Funktion zum Erfassen des hörbaren Momentanzustands müsste ausdrücklich anders benannt und definiert werden. Manuelles Ändern eines erfassten FM-/BRUTAL-Basisparameters deaktiviert MORPH für diesen Track sichtbar; A/B bleiben verfügbar. Dadurch reagiert die normale Sound-Bedienung direkt, statt hinter einem aktiven Morph-Ergebnis verborgen zu bleiben.

### 5.2 BRUTAL PERC

Die FM-Stimme besitzt bereits Carrier/Modulator, Modulationshüllkurve, Sweep und Feedback. Alle acht Parameter sind belegt: TUNE, DECAY, INDEX, RATIO, MDEC, SWEEP, FBK und VEL. Sie werden beim Trigger in den Stimmenzustand übernommen. Der Renderpfad liest diese Track-Parameter anschließend nicht erneut. [FM][fm]

| Modus | Vorgeschlagene Umsetzung | Ressourcenfolgen |
| --- | --- | --- |
| METAL | Begrenzte zusätzliche Verstimmung des Modulatorverhältnisses | Koeffizienten vorwiegend beim Hit bestimmen |
| GROWL | Grundtonbetonung; optional Beimischung eines Subtons innerhalb derselben Stimme | Vorhandenes ungenutztes Phasenfeld prüfen; zusätzliche Sinusauswertung messen |
| CHAOS | Begrenzte nichtlineare Formung des vorhandenen Feedbackpfads | Bestehenden Feedbackzustand nutzen; Rechengrenzen nachweisen |
| NOISE | Begrenzte Phasenstörung beziehungsweise Quantisierung | Eigener Audio-Zufallszustand; keine Kopplung an Pattern-RNG |

Zuerst wird die Belegung bisher ungenutzter modellabhängiger Felder geprüft. Eine Vergrößerung des globalen Stimmenobjekts wäre auf alle Stimmen wirksam und ist deshalb kein günstiger Ausgangspunkt. Das Objekt enthält bereits mehrere Phasen-, Inkrement- und Hüllkurvenfelder. [Stimmenzustand][voice]

MODE=OFF oder BRUT=0 verwendet den bisherigen FM-Rechenpfad exakt. Modus und Intensität werden zunächst beim Anschlag übernommen. Ausklingende Stimmen behalten ihren Modus und Feedbackzustand. BRUTAL-FM-CHAOS ist nicht automatisch bei CHARACTER=0 neutral; die Vorbildformeln dürfen diesen Vertrag nicht stillschweigend verletzen. [Vorbild-CHAOS][brutal-chaos]

### 5.3 SOUND MORPH

Die erste Stufe interpoliert Parameter für den nächsten Hit. A und B müssen dasselbe unterstützte Modell, denselben BRUTAL-Modus und kompatible diskrete Einstellungen besitzen. BRUTAL-Intensität darf interpoliert werden. Für FM wird RATIO zunächst nur zugelassen, wenn beide Endpunkte denselben Tabellenwert verwenden. Ein linear interpolierter Tabellenindex wäre kein stetiger Frequenzverlauf.

Eine kleine feste Tabelle definiert je Parameter: gültigen Bereich, Interpolationsart, diskret/stetig und Übernahmezeitpunkt. In der isolierten Morph-Stufe sind Endpunkte exakt: POS=0 ergibt A, POS=100 ergibt B, A=B bleibt identisch. Nachfolgende DNA, MOTION, PLOCK und LFO können den hörbaren Wert weiterhin verändern. Nur tatsächlich unterstützte Parameter werden interpoliert. SAMPLE-Slots, Oszillatorformen und Modellnummern sind keine beliebigen kontinuierlichen Zahlen. [FM-Verhältnisse][fm] · [Modellwechsel][model-follow]

Kontinuierliche Klangänderung in bereits ausklingenden FM-Stimmen ist eine zweite, gesonderte Stufe. Sie benötigt ausgewählte während des Renderns aktualisierbare Koeffizienten und Glättung. Jede Stimme muss ihre eigene Anschlagsnote behalten; der Track kann inzwischen eine neuere Note enthalten. Die Position zwischen A und B kann schon in Stufe eins stufenlos gewählt werden, während ihre hörbare Wirkung hitweise einsetzt. [Trigger/Render][drum-play]

### 5.4 SINE FOLDER

SINE wird als neuer gespeicherter INSERT-Typ angehängt. Das bisherige FOLD bleibt unverändert. Belegung: FOLD, SYMMETRY und BIAS auf der Typseite; MIX auf der vorhandenen zweiten Insert-Seite. Die Pegelkompensation beginnt als feste, konservative Kennlinie.

Der neue Typ verwendet die vorhandene Insert-Instanz, den bestehenden DC-Blocker und den vorhandenen Ablauf für Typwechsel und MIX-Fades. OFF beziehungsweise vollständig ausgefadetes MIX=0 bleiben exakt trocken. FOLD=0 wird nicht ohne ausdrückliche Implementierung als Identitätsfunktion bezeichnet: eine Sinuskennlinie ist auch bei einfacher Verstärkung eine Verformung. Stille, Bias und Ausklingen werden gesondert geprüft. [INSERT][insert]

Für die neue Kennlinie gilt ausdrücklich f(0)=0: ihr durch BIAS/SYMMETRY entstehender statischer Nullsignalwert wird kompensiert. Der DC-Blocker bleibt für signalabhängige Gleichanteile erhalten. Die Pegelbegrenzung berücksichtigt die durch diese Subtraktion veränderte Ausgangsspanne. Ein Bias-Regler soll beim Start aus digitaler Stille keinen selbst erzeugten Impuls liefern.

**Konkrete Integrationsstelle:** Dispatcher, Parameterbeschreibung und Graph klassifizieren heute alle IDs ab IT_PHASER als Swept-Effekte. Ein hinten angehängtes SINE benötigt an allen drei Stellen eine ausdrückliche Zuordnung. Die alten IDs werden nicht umnummeriert. [Enum][insert-enum] · [Dispatcher][insert-dispatch] · [Beschreibungen][insert-desc] · [Graph][insert-graph]

Der Folder liegt auf Track-Ebene nach vorgeschalteter Bearbeitung und erfasst auch den Sample-Layer. Die tonhöhenbezogene Begrenzung eines einzelnen BRUTAL-FM-Oszillators lässt sich dort nicht unverändert übernehmen. Der erste Umfang benutzt eine begrenzte Fold-Spanne und vorhandene Sinustabellen; globale Überabtastung wird dafür nicht vorausgesetzt. [Track-Signalweg][fx-path] · [Vorbild-Folder][brutal-folder]

### 5.5 MOVEMENT

Die Benutzeroberfläche zeigt 100 % als neutralen Wert. Intern kann ein vorhandenes Festkommaformat verwendet werden; der Unity-Pfad führt die bisherige Rechnung exakt aus. 0 % bedeutet keine zusätzliche Auslenkung der einbezogenen Modulationen. Der Bereich kann bis etwa 200 % reichen, mit Begrenzung auf den gültigen Zielbereich.

Einbezogen werden bestehende LFO-Beiträge zu ausdrücklich zugelassenen Klangzielen sowie neue TIMBRE-DNA-Auslenkungen. Ausgenommen sind Clock, Rate/Phase der LFOs, Microtiming, Swing, Gate, Velocity, VARY sowie SINK/SWELL/CUT. Direkte Basiswerte, A/B-Endpunkte und die manuelle Morph-Position werden nicht mitmultipliziert. Die zusätzliche Skalierung von TIMBRE DNA ist eine eigene GRIND-Entscheidung; im Vorbild werden ARP-Timbreoffsets nach der globalen MOVEMENT-Skalierung addiert. [LFO][lfo] · [BRUTAL MOVEMENT][brutal-movement]

Die anfängliche Zielmaske ist konkret:

| Zielgruppe | Von MOVE skalierte vorhandene LFO-Beiträge |
| --- | --- |
| Generisches FM | TUNE, DECAY, INDEX, MDEC, SWEEP und FBK |
| Gemeinsamer Track-Mix | LEVEL und PAN; nur vorhandene Modulation, nicht der Grundpegel |
| Resonator | Fine-Tune, DECAY, MIX, TONE, POSITION; STRUCTURE nur bei STRNG, PIPE und MODAL, ausdrücklich nicht bei CHORD |
| Filter und Effekte | CUTOFF, RESONANCE, SRR, DIST, CHORUS-/DELAY-/REVERB-Sends sowie INSERT MIX; INSERT A/B/C nur bei explizit zugelassenen stetigen Funktionen des wirksamen Typs: SOFT/HARD/FOLD/FUZZ und PHASR/FLANG/CHOR; bei CRUSH ausschließlich C (LPF), nicht A (BITS) oder B (RATE); bei SINE nur nach Prüfung der neuen Parameterbeschreibungen |
| Neue TIMBRE DNA | Auslenkung von INDEX, MDEC oder FBK |

RATIO, VEL, Modellnummern, Sample-Auswahl, diskrete Wellenformen und andere noch nicht freigegebene Modellparameter werden nicht über MOVE skaliert. Ihre vorhandenen LFO-Routings behalten ihr bisheriges Verhalten, auch bei MOVE=0 %. Die Zielmaske ist eine positive, modell-/typabhängige Allowlist: Vor der Skalierung werden der aktuelle Modus und die effektive Parameterbeschreibung (`track_desc`/`ins_desc`, einschließlich diskreter Auswahltabellen in `names`) sowie die tatsächliche DSP-Semantik geprüft. Ein numerischer Bereich 0..127 oder F_INT beweist keine Stetigkeit. CRUSH A/B wählen je 16 Stufen, CHORD STRUCTURE wählt eine Akkordtabelle; diese Ziele bleiben ausdrücklich außerhalb der MOVE-Skalierung. Bei einem Typübergang mit mehrdeutiger A/B/C-Bedeutung wird nicht skaliert, bis der wirksame Typ eindeutig freigegeben ist. Die Zielmaske wird später modellweise erweitert; ein Parameterindex allein ist keine Freigabe. [LFO-Ziele][lfo-dest] · [Modusabhängige Parameter][mode-desc]

Bei 0 % laufen die aufgrund ihrer Routings aktiven LFOs einschließlich ihrer Zufallsfolgen weiter. Andernfalls würde das Herunterdrehen den späteren Verlauf verändern. Gespeicherte Depth-Werte bleiben unverändert. LFO-Anzeige, Spezialpfade und tatsächlich gerenderte Auslenkung verwenden denselben Faktor. INIT und alte Projekte ohne neues Feld erhalten 100 %, nicht 0 %.

## 6. Memory Architecture

### 6.1 Belastbarer Ausgangspunkt

**Dokumentierter Wert:** Die aktuelle AGENTS.md nennt 95.160 von 98.304 Byte App-RAM. Daraus folgen rechnerisch 3.144 Byte Rest. Das ist kein in dieser Untersuchung neu gemessener Buildwert. Vor der ersten Implementierung ist die native CI-Zeile für den dann tatsächlich integrierten Commit maßgeblich. Die Buildprüfung hält außerdem mindestens 8.192 Byte Pool-Reserve frei. [Projektvorgaben][agents] · [Buildprüfung][build-memory]

App-RAM, .pool, RAM-Code und Stacks sind getrennt zu betrachten. Der normale Linker hat eigene Stackregionen mit Guard-Bereichen. Freie .bss-Bytes sind deshalb weder Stack-High-Water noch CPU-Reserve. Neue DNA- und BRUTAL-Funktionen teilen sich denselben verbleibenden Speicher; jede Planung darf ihn nicht erneut vollständig für sich beanspruchen. [Native Speicherkarte][app-ld]

### 6.2 Korrektur der Union-Rechnung

| Objekt | Rechnung | Größe |
| --- | --- | ---: |
| gm_pattern | 8×4 Timing + 8×64×4 Zellen + 64 Lock-Bytes | 2.144 B |
| gm_native_stage | 8×64×4 Steps + 4×8 Timing-Bytes | 2.080 B |
| Getrennte base/native-Objekte | 2.144 + 2.080 | 4.224 B |
| Vorhandene Union | max(2.144, 2.080) | 2.144 B |
| Bereits realisierte Ersparnis | 4.224 − 2.144 | **2.080 B** |

4.224 Byte sind der Bedarf vor Überlagerung, nicht die Ersparnis. Die Union ist bereits implementiert. Der separate Ausgabe- und Undo-Puffer bleiben zusätzlich bestehen. Die Typgrößen sind durch die Quellstruktur und die Vier-Byte-Prüfung des nativen Adapters abgesichert. [Typen][grind-types] · [Adapter][adapter] · [Union][runtime-union]

Puffer-Sharing wird nur auf nicht überlappende Lebensdauern angewendet. Die bestehende Beziehung base in WRITING, native Stage in READY/READING ist ein konkretes Beispiel. ACTIVE, UNDO und ein noch benötigter PENDING-Zustand können nicht auf dieser Grundlage miteinander überlagert werden.

### 6.3 Einordnung der BLE-Optimierungen

| Genannte Maßnahme | Befund am untersuchten Stand | Konsequenz für natives GRIND ohne BLE |
| --- | --- | --- |
| Chorus: 4.096 B .pool → .bss | Im BLE-Zweig vorhanden | Reine Umverteilung; hier bleibt der Chorus in .pool |
| Variationsbanken: 7.168 B → .ram_bss | Im BLE-Zweig vorhanden | Hier bleiben sie in .bss |
| Feste Adresse 0x01C01AD0..0x01C036CF | Historischer FIT-Messstand vom 27.10.2026 | Keine aktuelle freie Reserve; .ram_bss enthält inzwischen weitere Objekte |
| Vier Stacks von 4 auf 2 KiB | Historische Einsparung von 8.192 B | Aktuell existieren drei BLE-Task-Slots; sie gehören nicht zu diesem Native-Build |
| BT-Heap nach High-Water +25 % | Historische Dimensionierungsregel | Aktueller Fixwert ist 23.552 B; kein allgemein verfügbarer GRIND-Heap-Gewinn |
| LTO, Log-Stubs und Plugin-Option | Bestandteil des BLE-SDK-Builds | Kein bereits bewiesenes weiteres Einsparpotenzial im normalen GRIND-Core |

Quellen: [Chorus][chorus] · [Variationsbanken][variation] · [BLE-FIT-Historie][ble-fit] · [Task-Slots][ble-os] · [Heap][ble-heap] · [Buildoptionen][build-options]

Die historische Adresse wird im aktuellen BLE-Linker nicht fest reserviert; .ram_bss folgt dem tatsächlichen Ende von .ram_text. NOLOAD bedeutet nicht automatisch Nullinitialisierung oder Persistenz. BLE initialisiert ausgelagerte Objekte wie var_buf, ins und tl ausdrücklich im Bootpfad. Das normale app.ld, crt0.S sowie HAL/Loader sind laut Projektvorgaben eingefroren. Der vorliegende Entwurf setzt keine Änderung dieser Bereiche voraus. [BLE-Linker][ble-ld] · [Initialisierung][persist-boot] · [Projektvorgaben][agents]

LTO hat in der dokumentierten BLE-Historie große Mengen Code, Daten und Logtexte entfernt. Classic-/TWS-Sektionen wurden stark verkleinert, jedoch nicht pauschal vollständig beseitigt. Eine heutige Ersparnis muss als Differenz zweier vergleichbarer Ziel-Builds gemessen werden. Historische Einsparungen werden nicht nochmals zum aktuellen Rest addiert. [BLE-FIT][ble-fit]

### 6.4 Neue Zustände vollständig zählen

Ein vollständiger per-Step-Klangcache wäre teuer: bereits 8 Tracks × 64 Steps × 4 Parameter × 2 Byte = 4.096 Byte. Deshalb werden Konturen aus kompakten Rezepten ausgewertet und nur tatsächlich erforderliche Hit-/Blockwerte gehalten.

**Illustrative Bruttorechnung, keine zugesagte Implementierungsgröße:**

| Bestandteil eines angenommenen Metadatensnapshots | Rechnung | Bytes |
| --- | --- | ---: |
| Track-Rezepte | 8 × angenommene 24 B | 192 |
| A/B-Endpunkte | 8 × 2 × angenommene 20 B | 320 |
| Globale Zustandsdaten | angenommene 32 B | 32 |
| **Ein Snapshot** | | **544** |

Die angenommenen 20 Byte eines Endpunkts bestehen aus acht 16-Bit-Parametern plus je einem Byte für Modell, BRUTAL-Modus, BRUTAL-Intensität und Flags; die Schema-Version steht im gemeinsamen Header. Die genaue Feldbelegung ist erst bei der Implementierung festzulegen. Der 24-Byte-Rezeptansatz ist ebenfalls ein Planungswert; die je Track benötigten Morph-Felder POS und ON müssen darin nachweislich Platz finden oder als zusätzlicher Bedarf in der Ressourcenrechnung ausgewiesen werden.

| Gleichzeitig benötigte Zustände | Angenommener Bruttobedarf |
| --- | ---: |
| ACTIVE + STAGED/WRITING + LATEST-PENDING + UNDO | **2.176 B** |
| Zusätzlich ein unabhängiger DRAFT | **2.720 B** |

Gegen den dokumentierten Rest von 3.144 Byte würden damit rechnerisch nur 968 beziehungsweise 424 Byte übrig bleiben, noch vor zusätzlichem DSP-Zustand, Alignment, Verwaltungsdaten oder neuen Stack-Kopien. Diese Rechnung zeigt die Empfindlichkeit des Entwurfs; sie beweist nicht, dass alle sechs Funktionen bereits passen.

Der vorhandene Controller enthält active_config, pending.config und stage_req.config. Die Runtime hält außerdem lokale Requests während der Generierung. Eine große Beschreibung einfach in gm_config einzubauen vervielfacht deren Speicher und erhöht den Stackbedarf. Große neue Metadaten sollen deshalb über feste Slots mit klarer Eigentümerschaft und Revision transportiert werden. Der vorhandene Pattern-Undo von 2.080 Byte bleibt bestehen und ist nicht Teil der 544-Byte-Beispielbeschreibung. [Controller][controller] · [Runtime][runtime]

Auch der Generator besitzt bereits ein lokales gm_pattern next mit 2.144 Byte plus Regelmasken. Tatsächliche Stackframes hängen vom Zielcompiler ab. Ein bloß kleinerer .bss-Wert genügt daher nicht als Optimierungsnachweis. [Generator][generator]

### 6.5 Konkrete Speicherregeln

- Keine Heap-Allokation in Generator, Echtzeitpfad oder neuen UI-Zuständen.
- Keine zusätzliche vollständige Pattern-Historie und keine Kopien vollständiger track_t-Objekte für A/B.
- Vorhandene Preview und vorhandenen Canvas verwenden; neue Metadaten haben feste Kapazitäten.
- Konstante Modus-, Interpolations- und Kurventabellen als unveränderliche Daten halten und ihre tatsächliche Sektion im Zielartefakt prüfen.
- Zustände für BRUTAL bevorzugt innerhalb vorhandener modellabhängiger Stimmenfelder unterbringen; Feldbelegung dokumentieren.
- App-RAM, Pool, Flash, Stackframes/High-Water und Audiozeit getrennt ausweisen.
- Ein Speichergewinn zählt erst als gemessene Nettoänderung am gleichen Buildprofil. Voice-Cap und Pool-Mindestreserve bleiben erhalten.

## 7. Performance Architecture

### 7.1 Arbeit nach Häufigkeit aufteilen

| Zeitpunkt | Geeignete Arbeit | Begrenzung |
| --- | --- | --- |
| Main Loop / Benutzeraktion | Rezeptänderung, Konturvorschau, Generierung, Schutzprüfung, A/B-Capture, Formatierung | Keine langen IRQ-off-Abschnitte |
| Native Commit-Grenze | Bereits geprüfte Steps/Timing/Metadaten übernehmen | Keine Generierung oder Dateiverarbeitung |
| Pro Hit | Morph-Parameter, Timbre-Wert, BRUTAL-Koeffizienten und Stimmenstart | Keine Tabelle aller zukünftigen Steps aufbauen |
| Pro Audioblock | Vorhandene Modulationswerte, MOVE-Skalierung, nötige Glättung | Feste Anzahl Tracks/Slots; keine dynamische Suche |
| Pro Sample | Vorhandene FM-Oszillatoren plus gewählter Charakter, aktiver Insert | Keine Rezeptverwaltung, keine Speicherallokation, keine UI |

Einzelauswertungen können dort, wo gemessen sinnvoll, zu Block- oder Hit-Koeffizienten zusammengefasst werden. Diese Entscheidung wird nicht allein anhand eines Host-Benchmarks getroffen; entscheidend ist der gepinnte Zielcompiler auf dem FM-1.

**Quellbefund:** HALF_FRAMES beträgt 256; bei 44,1 kHz entsprechen das etwa 5,8 ms pro I²S-Halbpuffer. Diese Zeit gehört dem gesamten Audioablauf einschließlich bisheriger Arbeit und Interrupt-Anteilen. Sie ist kein freies Budget für neue Effekte. [Audio-Konstanten][audio-constants]

### 7.2 Repräsentative Belastungen

Die spätere Zielmessung umfasst dichte Patterns auf mehreren Tracks, Ratchets, überlappende lange Ausklänge, hohe FM-Stimmung und Feedback, aktive Inserts sowie gleichzeitige MIDI- und UI-Bedienung. Relevant sind höchste beobachtete Renderzeit und IRQ-Sperrzeit sowie bereits vorhandene Overrun-/Stack-Indikatoren. Durchschnittswerte allein decken kurze Überlastungen nicht ab.

Ein BRUTAL-Modus kann zusätzliche Sinusauswertungen oder Multiplikationen benötigen, obwohl er kein neues großes RAM-Array verwendet. SOUND MORPH benötigt ebenfalls Rechenarbeit. Umgekehrt reduziert eine kleinere Datenstruktur nicht automatisch die Audiozeit. Die drei Budgets RAM, Flash und Echtzeit werden deshalb getrennt bewertet.

Bestehende native Limits und Prüfungen gelten weiter. Globale Überabtastung, ein zweiter Renderer oder kleinere Voice-Caps werden nicht als automatische Ausweichlösung eingeplant. Wenn ein optionaler Modus zu teuer ist, werden zuerst dessen Berechnungsrate, Kennlinie oder Umfang angepasst.

## 8. Persistenz und Kompatibilität

Die vorhandene DNA-Spezifikation sieht zunächst externe Rezeptdateien und einen aktiven RAM-Zustand vor. Native Projekt- und Variationsformate sind eng belegt. Dieser Entwurf übernimmt deshalb für neue BRUTAL-Rezepte und A/B-Endpunkte zunächst **SESSION-Zustand mit explizitem Host-Export**. [DNA-Persistenz][dna]

Native SAVE sichert weiterhin die vom vorhandenen Format unterstützten Pattern- und Parameterdaten. Daraus folgt keine automatische Speicherung zusätzlicher Timbre-Rezepte, Endpunkte oder MOVEMENT-Einstellungen. Ein gespeicherter Rhythmus kann nach einem Neustart wieder spielbar sein, obwohl die zusätzliche Klangentwicklung ohne erneut geladenes Rezept fehlt. Die neuen Seiten kennzeichnen diesen Zustand sichtbar.

Ein vollständiger externer Export muss Basisprojekt beziehungsweise benötigte Basisparameter, Version, wirksame Schlüssel, Layer-Locks, Timbre-Rezepte, A/B-Endpunkte einschließlich ihrer Gültigkeit, die aktuelle Morph-Position POS (0..100) und den Aktivierungszustand ON (OFF/ON) je Track, BRUTAL-Einstellungen und MOVE enthalten. POS und ON werden auch bei deaktiviertem MORPH exportiert; dieselben A/B-Endpunktdaten können bei unterschiedlichem POS oder ON unterschiedliche aktive Klangwerte ergeben. Der Import prüft vor jeder Übernahme die Version, Feldgrenzen, Modell-/Moduskompatibilität sowie die Endpunktgültigkeit (ON nur bei zwei gültigen kompatiblen Endpunkten). Nach erfolgreicher expliziter Übernahme stellt er POS und ON zusammen mit A/B und dem zugehörigen Rezept wieder her; ein ungültiges Paket wird ohne Teiländerung abgelehnt. Nur tatsächlich implementierte Export-/Importfunktionen dürfen als Produktfähigkeit beworben werden. Die spätere Firmware-Integration muss diesen Export oder eine nachgewiesene native Speicherung bereitstellen, bevor ein vollständiger Performance-Roundtrip zugesagt wird.

Projekt-/Slotwechsel, Restore oder inkompatibler Modellwechsel invalidieren betroffene Endpunkte und Rezeptidentitäten; MOVE erhält beim definierten Reset den neutralen Wert. SOUND-Capture und Lock-Umschaltung erzeugen keine automatischen Flash-Schreibvorgänge.

Alte Projekte müssen in der neuen Firmware mit neutralen Defaults weiterlaufen. Eine alte Firmware kann dagegen einen neu gespeicherten SINE-Typ nicht automatisch verstehen. Beim Append der Insert-ID müssen Parameterprüfung, Speicherung, Backup und Editor gemeinsam betrachtet werden. Rückwärtskompatibilität der neuen Firmware bedeutet nicht, dass neue Sounds auf alter Firmware identisch funktionieren.

## 9. Integrationsreihenfolge und gezielte Nachweise

| Schritt | Konkretes Ergebnis | Wesentlicher Nachweis |
| --- | --- | --- |
| BE00 – Ressourcenbasis | Native Map und RAM/Pool/Flash/Stack-/Audiowerte für einen festen Commit; Zustandsinventar | Gemessener Ausgangspunkt, keine mehrfach gezählten Reserven |
| BE01 – LOCKS | Feldgenaue Sperren je Track; Revisionsinvalidierung; kohärentes Undo | Nur die gewählte Ebene verändert sich; geschützte OFF-Zellen bleiben OFF; dormante Steps bleiben vollständig erhalten; fremder manueller Edit invalidiert UNDO/REDO; Live-REC im Fenster vor der IRQ-Sperre darf nicht überschrieben werden |
| BE02 – TIMBRE DNA | Feste Rezeptgröße, deterministische Konturen, Ziel-Kompatibilität | Gleiches Rezept/Basis/Version ergibt gleiche Hitwerte; AMT=0 neutral; TIMBRE-only verändert keine nativen Step-/Timing-Bytes (alle 8 × 64 Steps) |
| BE03 – BRUTAL PERC | MODE/BRUT und klar zugeordnete Stimmenfelder | OFF/0 entspricht alter FM; Pegel, DC, Überläufe und vollständiges Abklingen |
| BE04 – SINE FOLDER | Neuer Typ mit Dispatch, Beschreibungen, Graph und vorhandenen Fades | Trockener Bypass; schnelle Wechsel und Stille ohne alten Zustand |
| BE05 – SOUND MORPH | Kompatibilitätsmatrix, A/B-Capture, hitweiser Morph | Exakte Endpunkte; diskrete Konflikte sichtbar; PLOCK/MOTION/LFO unverändert gültig; bei bereitgestelltem Export/Import Roundtrip mit unterschiedlichen kompatiblen Endpunkten, POS und ON/OFF sowie geprüftem tatsächlichem Hitwert |
| BE06 – MOVEMENT | Globaler Beitragsfaktor inklusive Anzeige und Sonderpfaden | 100 % exakt neutral; 0 % verändert keine LFO-Phase oder Zufallsfolge; CRUSH BITS/RATE und CHORD STRUCTURE behalten bei 0/100/200 % ihren Legacy-LFO-Beitrag, erlaubte stetige Ziele skalieren |

BE05 hängt von der Parameterauflösung und den Schutzregeln ab, nicht grundsätzlich von SINE. MOVEMENT kann parallel vorbereitet werden, sobald die einbezogenen Beiträge feststehen. Die Nummerierung beschreibt eine sinnvolle Lieferfolge, keine unnötig starre Abhängigkeit.

Die Umsetzung verwendet die bestehenden Host-, WASM-, Native- und Performance-Prüfungen. Neue Tests richten sich auf die konkreten Risiken: Zustandsbesitz, Lock-Isolation, unveränderte Legacy-Ausgabe, Endpunktgenauigkeit, Bias/DC, Integergrenzen, Typwechsel und Ressourcenänderungen. Host-Ergebnisse belegen keine auf Hardware gemessenen Audiofristen. [Projektvorgaben][agents]

**Abnahmeszenario:** Einen Blastbeat mit belegten Steps jenseits der später verkürzten Tracklänge vorbereiten; native Step- und Timingdaten aller Tracks vorab sichern. Rhythmus und Timing sperren; ausschließlich TIMBRE mutieren; am nativen END erscheint der neue Klangverlauf ohne Änderung eines nativen Steps oder Timingfelds. UNDO und REDO unmittelbar danach prüfen (Pattern und Rezeptidentität kohärent). Track wieder verlängern: die zuvor dormanten Noten einschließlich ihrer Attribute sind unverändert vorhanden. Nach einer weiteren Mutation einen manuellen Step-Edit durchführen: UNDO/REDO ist nicht mehr verfügbar und der Edit bleibt erhalten. MORPH bewegen, MOVE auf 0 und wieder 100 % stellen und einen Slotwechsel durchführen. Bei bereitgestelltem Performance-Export zusätzlich die folgenden gezielten Roundtrip-Prüfungen ausführen; ungültiges ON ohne zwei kompatible Endpunkte wird ohne Teiländerung abgelehnt. Pattern, wirksames Rezept, Statusanzeige und gespeicherte Automation müssen dabei konsistent bleiben.

**Gezielte Undo-Race-Prüfung (BE01):** Einen gültigen Undo-Snapshot erzeugen und einen noch unbelegten nativen Step als Ziel eines Live-REC-Hits wählen. Über einen deterministischen Host-Test-Hook den tatsächlichen `rec_hit`-Schreibpfad genau vor Eintritt in den Undo-Swap-Guard ausführen lassen (nach einer etwaigen ungeschützten Vorprüfung). Der aufgenommene Step muss gegenüber dem gesicherten Vorzustand nachweislich verändert sein; UNDO/REDO muss im Guard abgelehnt werden und Step, zugehöriges Timing und nach dem Hit gültige Rezept-/Revisionsdaten erhalten. Kein Undo-bedingter Richtungswechsel oder weiterer Revisionseffekt. Gegenprobe ohne konkurrierenden Edit: UNDO und unmittelbar danach REDO stellen die jeweiligen gesicherten Zustände wieder her. Prüfe zusätzlich die Gegenreihenfolge: Ein nach dem erfolgreichen geschützten Tausch eintreffender REC-Hit bleibt erhalten und invalidiert den verbleibenden Snapshot. Die Race-Prüfung muss bei alleiniger Vorprüfung (ohne entscheidende Prüfung im Guard) fehlschlagen. Prüfe zusätzlich den Ersatzfall: Die Undo-Aktion hält die beobachtete Snapshot-Identität fest; vor Eintritt in den Guard ersetzt ein nativer Commit (`gm_runtime_commit`) den Snapshot durch einen anderen gültigen. UNDO/REDO muss im Guard abgelehnt werden, ohne Tausch, Richtungswechsel oder Revisionseffekt; der Test muss fehlschlagen, wenn nur `undo_ok` geprüft wird.

**Gezielter Morph-Roundtrip (BE05, sobald Export/Import bereitsteht):** Gleichheit A=B bleibt ein separater Invarianztest mit POS-/ON-Anzeige, kann aber die Wiederherstellung der Position im Audiopfad nicht beweisen. Verwende zusätzlich kompatible FM-Endpunkte mit absichtlich unterschiedlichem INDEX (z. B. A=20, B=100, gleiche übrige Parameter, identischer RATIO und BRUTAL=OFF), POS=37 und einen davon verschiedenen ruhenden INDEX-Basiswert (z. B. 10). Deaktiviere TIMBRE-DNA, MOTION, PLOCK und LFO für diesen Test und triggere dieselbe Note mit Velocity 96. Berechne den erwarteten Zwischenwert unabhängig von Exporter, Importer und getesteter Morph-Auflösung anhand der festgelegten Interpolationsart und Integer-Rundung; wähle die Fixture so, dass er sich vom OFF-Basiswert und den Werten bei POS=0, 50 und 100 unterscheidet. Exportiere ON und OFF getrennt mit denselben unterschiedlichen Endpunkten und POS=37. Setze den Zielzustand vor jedem Import auf absichtlich andere Position/Aktivierung und abweichende Endpunkte. Nach expliziter Übernahme prüfe Anzeige und importierte A/B-Daten, dann den beim tatsächlichen FM-Hit gelatchten INDEX (heute `v->x[0]` in `fm_trigger`, vom Renderer verwendet): bei ON exakt der unabhängig berechnete Zwischenwert, bei OFF exakt der ruhende Basiswert. OFF behält die gespeicherte Position; erneutes Einschalten ohne neue CAP-Aktion muss den Zwischenwert wiederherstellen. Der Test muss fehlschlagen, wenn der Import POS durch einen Default ersetzt, nur die Anzeige aktualisiert oder ON ignoriert. [FM][fm]

## 10. Offene technische Nachweise

Der Entwurf entscheidet Produktumfang und Verhalten. Vor einer Implementierungszusage bleiben konkrete Messungen und Layoutentscheidungen offen: die endgültige Bytegröße der Rezepte und Endpunkte, tatsächlich gleichzeitig benötigte Slots, verfügbare Stimmenfelder je BRUTAL-Modus, Zielkosten des zusätzlichen GROWL-Anteils, gültige Interpolationsarten und ein vollständiger Export-/Importvertrag. Diese Punkte werden in BE00 beziehungsweise der jeweils betroffenen Integration aufgelöst.

Die wichtigste Konsequenz ist bereits festgelegt: BRUTAL EVOLUTION muss seine neuen Zustände sparsam in die vorhandenen Besitz- und Parameterpfade integrieren. Historische BLE-Umbauten, die bereits vorhandene Union und dokumentierte Restbytes sind keine neuen Reserven.

## Quellen

Alle GRIND-Dateilinks beziehen sich auf denselben oben genannten Commit. Bei historischen Tabellen ist deren Datum beziehungsweise historischer Charakter im Text genannt. Die Links belegen bestehenden Code und Dokumentation; vorgeschlagenes Verhalten ist als Entwurfsentscheidung gekennzeichnet.

[agents]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/AGENTS.md
[fm]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/dm_fm.c
[insert]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/insert.c
[lfo]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/lfo.c#L163-L257
[runtime]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/runtime.c
[runtime-reset]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/runtime.c#L35-L43
[dna]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/docs/superpowers/specs/2026-11-27-grind-dna-engine-design.md
[panel]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/docs/manual/03-panel.md#L6-L36
[navigation]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ui.c#L288-L327
[pages]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/pages.c#L21-L106
[footer]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ui_draw.c#L2553-L2560
[actions]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/ui.c#L247-L277
[input-priority]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ui_input.c#L1562-L1598
[keep-gesture]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ui_input.c#L1481-L1518
[grind-ui]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/ui.c#L2-L6
[layout]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ui.c#L50-L66
[knob-draw]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ui_draw.c#L373-L425
[gfx]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/gfx.c#L3-L35
[redraw]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ui_draw.c#L24-L40
[runtime-keep]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/runtime.c#L138-L184
[rng]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/rng.c#L13-L20
[controller]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/controller.h
[runtime-undo]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/runtime.c#L84-L135
[preview]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/ui.c#L145-L176
[motion]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/motion.c#L4-L175
[motion-rest]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/motion.c#L296-L304
[voice]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/core.h#L144-L170
[brutal-chaos]: https://github.com/hiroikezawa/brutal-fm/blob/d2081886152cc7fb716cdeb476ee56e9e4437cc0/firmware/brutal/bfm_dsp.c#L487-L541
[model-follow]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/drum_core.c#L28-L55
[drum-play]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/drum_core.c#L147-L188
[insert-enum]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/core.h#L65-L66
[insert-dispatch]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/insert.c#L246-L277
[insert-desc]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/params.c#L185-L205
[insert-graph]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ui_draw.c#L1880-L1919
[fx-path]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/fx.c#L796-L821
[brutal-folder]: https://github.com/hiroikezawa/brutal-fm/blob/d2081886152cc7fb716cdeb476ee56e9e4437cc0/firmware/brutal/bfm_dsp.c#L395-L532
[brutal-movement]: https://github.com/hiroikezawa/brutal-fm/blob/d2081886152cc7fb716cdeb476ee56e9e4437cc0/firmware/brutal/bfm_dsp.c#L314-L429
[build-memory]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/tools/build.py#L394-L415
[app-ld]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/app.ld#L4-L33
[grind-types]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/grind.h#L8-L62
[adapter]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/seq_adapter.c#L10-L12
[runtime-union]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/runtime.c#L13-L33
[chorus]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/fx.c#L7-L15
[variation]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/project.c#L143-L154
[ble-fit]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/docs/ble/FIT.md
[ble-os]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ble/ble_os.c#L16-L41
[ble-heap]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/ble/ble.c#L37-L43
[build-options]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/tools/build.py#L228-L281
[ble-ld]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/app_ble.ld#L49-L58
[persist-boot]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/project.c#L2110-L2116
[generator]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/core.c#L127-L158
[audio-constants]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/core.h#L35-L40
[lfo-dest]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/lfo.c#L37-L47
[native-export]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/seq_adapter.c#L75-L90
[native-commit]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/grind/runtime.c#L86-L105
[mode-desc]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/params.c#L182-L209
[live-rec]: https://github.com/DaFum/grind-machine/blob/7b45e8197c45e5c718d670ffa42481724811c76e/firmware/src/seq.c#L420-L458
