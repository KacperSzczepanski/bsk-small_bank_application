Kacper Szczepański

Mała aplikacja bankowa, w której klienci (po zalogowaniu przy użyciu PAM) widzą tylko swoje pliki, podczas gdy pracownicy mają dostęp do wszystkich i jako jedyni mogą je edytować.

Zaczynając od naprostszych rzeczy:

uzytkownicy.txt - plik z loginami, rolami i pełnymi imonami użytkowników (z założeniem, że format jest poprawny)
init.sh - uprawnienia 777, tworzy 2 grupy użytkowników, na podstawie uzytkownicy.txt tworzy użytkowników, przydziela im role oraz hasła (takie same jak loginy)
        - następnie stworzone są foldery deposits i credits oraz przydzielone grupom zostają odpowiednie uprawnienia
Dockerfile - system ubuntu 18.04, zainstalowane: gcc, make, acl, pam, sudo, openssh (server), python3, pip3, django, python-pam
           - do pliku /etc/ssh/sshd_config dodane zostają linie wymuszające uruchomienie apliakcji pracownika, gdy on łączy się przez ssh
           - uruchomiony zostaje init.sh oraz serwer django na porcie 8080

klient - podstawowy projekt django bez wyglądu, ale z logowaniem metodą pam
       - przeszukuje on obydwa foldery w poszukiwaniu plików, filtruje je i wyświetla odpowiednie klientowi w przeglądarce

pracownik - dostarczone są officerApp.c, makefile, ale plik binarny (offierApp, również dostarczony) jest najnowszą skompilowaną wersją z uprawnieniami 777
          - officerApp.c wykonuje wszystko to co powinien według treści
          - początkowo było mnóstwo wycieków pamięci, ale z valgrindem udało się je "wszystkie" znaleźć (w niektórych miejcach są notatki)
          - "wszystkie" ponieważ pam generuje wycieki pamięci, co do których nie jestem przekonany czy da się je naprawić

jak uruchomić:
0) w razie potrzeby użyć docker ps lub docker container ls

1) stworzenie obrazu (w pliku z Dockerfile) - docker build .
może to potrwać nawet 15-20 minut

2) docker run -p 8080:8080 <img>
django powinien się po paru chwilach uruchomić

3) docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' <container_id>
stąd dostajemy informację o adresie ip serwera kontenera

wtedy: ssh pracownik@adres-ip

po zalogowaniu powinnien uruchomić się offierApp

3.1) Gdy program się uruchomi, to nie z poprawnymi uprawnieniami (chyba), co spowoduje, że dostaniemy naruszenie ochorny pamięci przy próbie stworzenia pliku. Nie udało mi się tego obejść.
Próbowałem uprawnienia 4755, 777, nawet dodałem grupę pracownika do grupy sudoersów, ale żadnych efektów.

3.2) Żeby przetestować aplikację pracownika z uprawnien roota, należy użyć
docker exec -it <container_id> /officerDir/officerApp


W razie chęci testowania wszystkiego poza dockerem należy użyć sudo przy uruchamianiu serwera django, jak i (a być może i nie) uruchamiając aplikację pracownika.
