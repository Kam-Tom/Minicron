# Minicron
Minicron: Lekki program uruchamiający programy o określonych porach.

W folderze znajduje się pliki:
- `Makefile`
- `minicron.c`
- `list.c`
- `task.c`
- `list.h`
- `task.h`
- `a.txt`
- `b`

## Użycie
Aby skorzystać z Minicron, wykonaj następujące polecenie:
```bash
./minicron <taskfile> <outfile>
```
Gdzie taskfile to plik z zadaniami do wykonania w formacie
```bash
<hour>:<minutes>:<command>:<mode>
```
Np:
```bash
11:20:ls -al:2
```

## Oznaczenia mode:
- 0 - użytkownik chce otrzymać treść, jaką polecenie wypisało na standardowe wyjście
(stdout)
- 1 - użytkownik chce otrzymać treść, jaką polecenie wypisało na wyjście błędów (stderr).
- 2 - użytkownik chce otrzymać treść, jaką polecenie wypisało na standardowe wyjście i wyjście błędów.

## Funkcjonalność 
- Odpalanie programów o odpowiednim czasie.
- Obsługa potoków np. ls -l | wc -l.
- Ponowny odczyt pliku po otrzymaniu sygnału SIGUSR1.
- Wypis do logu systemowego listy zadań do wykonania po otrzymaniu SIGUSR2.
(Np. przez kill -SIGUSR2 <pid>)
- Wyłączenie programu po otrzymaniu SIGINT.
- Zapis informacji o odpaleniu programu w logu systemowym.
- Zapis kodu zwrotnego zakończonych procesów w logu systemowym.
- Zakończenie programu po wykonaniu wszystkich zadań.
- Zasypianie w oczekiwaniu na czas odpalenia kolejnego zadania.

## Użycie Makefile
### Kompilacja:
```bash
make
```
Test na plikach o nazwach a.txt i b:
```bash
make test
```
Test valgridem (tylko z zainstalowanych valgridem w systemie)
```bash
make leaks
```
