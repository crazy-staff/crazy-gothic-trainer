# Jak testować i zgłaszać problemy

Dzięki, że pomagasz! Wszystko, czego potrzebujesz, jest w grze — w **Centrum testera**.

## 1. Włącz tryb testera

1. W grze naciśnij **Insert** → w pasku bocznym na dole **Konfiguracja**.
2. Włącz **Tryb testera** i kliknij **Otwórz Centrum testera** (albo **Testy** w pasku bocznym).
3. W prawym górnym rogu Centrum wpisz **swój nick** — każde zgłoszenie jest nim podpisane.

W trybie testera przy każdej opcji w menu pojawia się:
- **kropka** — status opcji od autora (zielona OK, żółta BUG, czerwona CRASH, fioletowa TEST = nowe, do sprawdzenia). Najedź na nią, żeby zobaczyć historię.
- **flaga** — otwiera formularz zgłoszenia od razu do tej opcji. Pomarańczowa flaga = są już znane problemy.

## 2. Testuj

Zakładka **Checklista**: przy każdej opcji kliknij **Działa / Częściowo / Nie działa**.
- **Kropka autora** filtruje po statusie od autora — w buildach alfa testuj głównie **TEST (nowe)** i **BUG**, nie wszystko od nowa.
- Nic nie jest nadpisywane: każda ocena trafia do historii (z numerem buildu), więc w kolejnych wersjach widać, co się zmieniło.
- Po „Częściowo” / „Nie działa” otworzy się formularz — opisz, co poszło nie tak (opcjonalnie, ale bardzo pomaga).
- Kliknij licznik wpisów przy opcji, żeby zobaczyć **wszystko, co już o niej wiadomo**: uwagi autora, znane problemy z GitHuba i wszystkie wcześniejsze zgłoszenia.

Zakładka **+ Nowe zgłoszenie** — formularz dla błędu, crasha, propozycji albo pytania:
- **Czego dotyczy** — wybierz opcję z listy (jest wyszukiwarka) albo „Ogólne”. Pod spodem zobaczysz znane problemy tej opcji: sprawdź, czy twój już tam nie jest.
- **Kroki** — co robiłeś po kolei. To najważniejsze pole: z dobrych kroków da się błąd powtórzyć i naprawić.
- **Dołącz stan gry** (zostaw włączone) — trainer sam dopisze świat, pozycję, włączone opcje i ostatnie zdarzenia.
- **Załączniki**: zrób zrzut (Win+Shift+S albo PrtScr) i naciśnij **Ctrl+V** w formularzu, albo „Dodaj plik z dysku”. Zrzuty trafiają do raportu (.zip).
- Zgłaszać można też **całą kategorię, podkategorię, kartę albo aspekt** (UI, UX, wydajność…): flaga przy nagłówku karty / podkategorii albo wybór w polu „Czego dotyczy”.

Zakładka **Dziennik** pokazuje, co trainer zanotował w tej sesji (włączenia opcji, akcje, złapane błędy). Dziennik sam dołącza się do zgłoszeń.

**Crash?** Uruchom grę ponownie — trainer wykryje, podczas czego gra padła, i sam przygotuje szkic zgłoszenia w zakładce **Zgłoszenia**. Dopisz tylko, co robiłeś.

## 3. Wyślij

Zakładka **Wyślij** — wybierz jeden sposób:

| Sposób | Jak |
|---|---|
| **GitHub** (najlepiej) | „Otwórz niewysłane na GitHubie” — każde zgłoszenie otwiera się w przeglądarce jako wypełniony formularz, klikasz **Submit new issue**. Potrzebne darmowe konto GitHub. Przy crashu dołącz plik raportu (niżej). |
| **Plik raportu** | „Eksportuj niewysłane” — powstaje jeden plik (.txt, a z załącznikami .zip) w `Gothic\system\Autorun\G1CrazyTrainer_raporty\` ze wszystkim: zgłoszenia, oceny, dziennik, błędy z logu, dane o komputerze. Wyślij go autorowi (Discord / mail). |
| **Discord** | „Kopiuj na Discorda” — krótki tekst niewysłanych zgłoszeń do wklejenia na kanał. |

Wysłane zgłoszenia są oznaczane i nie wysyłają się drugi raz.

## Dobre zgłoszenie

- Jedno zgłoszenie = jeden problem.
- Tytuł mówi, co się dzieje: „Deszcz zaklęć wyłącza grę przy 10 celach”, a nie „nie działa”.
- Kroki krok po kroku, od wczytania gry.
- Jeśli problem już jest na GitHubie — daj 👍 i dopisz komentarz zamiast nowego zgłoszenia. Propozycje z największą liczbą 👍 robię najpierw.

## Prywatność

Raport zawiera: nick, wersję Windowsa, nazwę procesora i karty graficznej, ilość RAM, rozdzielczość, listę plików DLL z folderu gry, ustawienia trainera i jego log. Nic więcej. Plik jest zwykłym tekstem — możesz go przejrzeć przed wysłaniem.
