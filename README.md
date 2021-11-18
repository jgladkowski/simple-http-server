# Simple HTTP Server (exact description available only in Polish)
<div id="intro" class="box py-3 generalbox boxaligncenter"><div class="no-overflow"><p>Zadanie polega na napisaniu prostego serwera protokołu HTTP, z wąskim zakresem obsługiwanego wycinka specyfikacji protokołu HTTP/1.1 oraz specyficznym zachowaniem w przypadku niedostępności zasobu żądanego przez klienta.</p>
<p></p>
<p>Program serwera będzie uruchamiany następująco:</p>
<p></p>
<p>serwer &lt;nazwa-katalogu-z-plikami&gt; &lt;plik-z-serwerami-skorelowanymi&gt; [&lt;numer-portu-serwera&gt;]</p>
<p></p>
<p>Parametr z nazwą katalogu jest parametrem obowiązkowym i może być podany jako ścieżka bezwzględna lub względna. W przypadku ścieżki względnej serwer próbuje odnaleźć wskazany katalog w bieżącym katalogu roboczym.</p>
<p>Parametr wskazujący na listę serwerów skorelowanych jest parametrem obowiązkowym i jego zastosowanie zostanie wyjaśnione w dalszej części treści zadania (<a href="#skorelowane">Skorelowane serwery HTTP</a>).</p>
<p>Parametr z numerem portu serwera jest parametrem opcjonalnym i wskazuje numer portu na jakim serwer powinien nasłuchiwać połączeń od klientów. Domyślny numer portu to 8080.</p>
<p>Po uruchomieniu&nbsp;serwer powinien odnaleźć wskazany katalog z plikami&nbsp;i rozpocząć nasłuchiwanie na połączenia TCP od klientów na wskazanym porcie. Jeśli otwarcie katalogu, odczyt z katalogu, bądź otwarcie gniazda sieciowego nie powiodą się, to
    program powinien zakończyć swoje działanie z kodem błędu EXIT_FAILURE.</p>
<p></p>
<p>Serwer po ustanowieniu połączenia z klientem oczekuje na żądanie klienta. Serwer powinien zakończyć połączenie w przypadku przesłania przez klienta niepoprawnego żądania. W takim przypadku serwer powinien wysłać komunikat o błędzie, ze statusem 400, a
    następnie zakończyć połączenie. Więcej o wysyłaniu komunikatu błędu w dalszej części treści. Jeśli żądanie klienta było jednak poprawne, to serwer powinien oczekiwać na ewentualne kolejne żądanie tego samego klienta lub zakończenie połączenia przez
    klienta.
</p>
<h2 id="h.yfmn1p45lpef"><a name="format"></a>Format komunikacji</h2>
<p>Wszystkie komunikaty wymieniane pomiędzy klientem a serwerem powinny mieć postać:</p>
<p></p>
<pre><tt>HTTP-message &nbsp; = start-line</tt></pre>
<pre>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; <tt>*( header-field CRLF )</tt></pre>
<pre>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; <tt>CRLF</tt></pre>
<pre>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; <tt>[ message-body ]</tt></pre>
<p></p>
<p>Taką samą, powyższą postać <tt>HTTP-message</tt>, powinny przyjmować wszystkie komunikaty wysyłane przez serwer do klienta jak również komunikaty klienta wysyłane do serwera (więcej o formacie wiadomości można przeczytać w rozdziale “3. Message format”
    w oficjalnej dokumentacji dla protokołu HTTP, pod adresem: <a href="https://www.rfc-editor.org/rfc/inline-errata/rfc7230.html">https://www.rfc-editor.org/rfc/inline-errata/rfc7230.html</a>).
</p>
<p>Jeśli serwer otrzyma żądanie niezgodne z powyższą specyfikacją, to powinien odpowiedzieć błędem numer 400. Szczegóły konstruowania odpowiedzi serwera opisane zostały w dalszej części treści zadania.</p>
<h3 id="h.n59ef4cuijll">Pierwsza linia komunikatu - <tt>start-line</tt></h3>
<p>Ponieważ implementujemy server, to spośród wysłanych do nas komunikatów akceptujemy tylko komunikaty, w których <tt>start-line</tt>&nbsp;jest postaci <tt>request-line</tt>. Żądanie klienta zatem powinny rozpoczynać się od <tt>status-line</tt>&nbsp;postaci
    <tt>request-line</tt>&nbsp;takiej, że:
</p>
<p></p>
<pre>request-line&nbsp;= method SP request-target SP HTTP-version CRLF</pre>
<p>gdzie:</p>
<ul>
    <li><tt>method</tt>&nbsp;- jest tokenem wskazującym metodę żądaną do wykonania na serwerze przez klienta. Metody akceptowane przez nasz serwer to: <tt>GET</tt>&nbsp;oraz <tt>HEAD</tt>. Nazwa metody jest wrażliwa na wielkość znaków. Więcej o obsłudze metod
        w dalszej części niniejszej treści.
    </li>
    <li><tt>SP</tt>&nbsp;- jest pojedynczym znakiem spacji.</li>
    <li><tt>request-target</tt>&nbsp;- identyfikuje zasób na którym klient chciałby wykonać wskazaną wcześniej metodę. Nazwa zasobu nie może zawierać znaku spacji. Dla uproszczenia zakresu zadania przyjmujemy, że nazwy plików mogą zawierać wyłącznie znaki
        [a-zA-Z0-9.-], a zatem nazwa zasobu może zawierać wyłącznie znaki [a-zA-Z0-9.-/].</li>
    <li><tt>HTTP-version</tt>&nbsp;- w naszym przypadku jest to zawsze ciąg znaków: <tt>HTTP/1.1</tt></li>
    <li><tt>CRLF</tt>&nbsp;- ciąg dwóch znaków o wartościach ASCII równych 13 i 10.</li>
</ul>
<p></p>
<p>Odpowiedź serwera powinna mieć także postać <tt>HTTP-message</tt>, jednak w przypadku komunikatów z serwera <tt>start-line</tt>&nbsp;powinna przybrać postać <tt>status-line</tt>&nbsp;takiej, że:</p>
<p></p>
<p><tt>status-line = HTTP-version SP status-code SP reason-phrase CRLF</tt></p>
<p>gdzie:</p>
<ul>
    <li><tt>HTTP-version</tt>&nbsp;- w naszym przypadku, ponownie, zawsze będzie to ciąg znaków: HTTP/1.1.</li>
    <li><tt>SP</tt>&nbsp;- jest pojedynczym znakiem spacji,</li>
    <li><tt>status-code</tt>&nbsp;- jest numerem reprezentującym kod statusu odpowiedzi serwera. Status może wskazywać na poprawne wykonanie akcji po stronie serwera bądź jej niepowodzenie. Więcej o obsługiwanych przez nas kodach w dalszej części treści zadania.</li>
    <li><tt>reason-phrase</tt>&nbsp;- jest opisem tekstowym zwróconego statusu. Nasz serwer powinien zawsze uzupełnić to pole niezerowej długości napisem opisującym powód błędu.</li>
    <li><tt>CRLF</tt>&nbsp;- ciąg dwóch znaków o wartościach ASCII równych 13 i 10.</li>
</ul>
<h3 id="h.m7ij1u80kq45">Nagłówki żądania i odpowiedzi - <tt>header-field</tt></h3>
<p>W dalszej części naszego formatu wiadomości, wymienianych pomiędzy naszym serwerem a klientami, następuje sekcja nagłówków. Sekcja składa się z zera lub więcej wystąpień linii postaci:</p>
<p></p>
<p><tt>header-field &nbsp; = field-name ":" OWS field-value OWS</tt></p>
<p>gdzie:</p>
<ul>
    <li><tt>ields-name</tt>&nbsp;- jest nazwą nagłówka, nieczułą na wielkość liter. W dalszej części zostaną wymienione nagłówki obsługiwane przez naszą implementację serwera.
    </li>
    <li>":"&nbsp;- oznacza literalnie znak dwukropka.</li>
    <li><tt>OWS</tt>&nbsp;- oznacza dowolną liczbę znaków spacji (w szczególności także brak znaku spacji).</li>
    <li><tt>field-value</tt>&nbsp;- jest wartością nagłówka zdefiniowaną adekwatnie dla każdego z dozwolonych nagłówków protokołu HTTP. W dalszej części treści zostaną opisane także oczekiwane wartości i znaczenie nagłówków.</li>
</ul>
<p>Przypomnienie: Po wysłaniu wszystkich nagłówków należy zawsze zakończyć tę sekcję znakami <tt>CRLF</tt> (patrz: <a href="#format">Format komunikacji</a>).</p>
<h3 id="h.ogpbnudijnrx">Treść komunikatu - <tt>message-body</tt></h3>
<p>Ostatnim elementem w zdefiniowanym formacie komunikatów, wymienianych pomiędzy serwerem a klientem, jest ich treść (ciało):</p>
<p>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; <tt>[ message-body ]</tt></p>
<p></p>
<p>Występowanie treści w komunikacie determinuje wystąpienie wcześniej nagłówka <tt>Content-Length</tt>. Ponieważ w założeniu nasza implementacja obsługuje tylko metody <tt>GET</tt> i <tt>HEAD</tt>, to nasz serwer ma prawo z góry odrzucać wszystkie komunikaty
    klienta, które zawierają ciało komunikatu. Odrzucenie żądania klienta powinno skutkować wysłaniem przez serwer wiadomości z błędem numer 400.</p>
<p>Natomiast nasz serwer wysyłając treść komunikatu musi wysłać także nagłówek <tt>Content-length</tt> z odpowiednią wartością, szczegóły w dalszej części treści niniejszego zadania.</p>
<h2 id="h.xwzwc1hwjdn2">Zasoby serwera</h2>
<p>Nasz serwer podczas startu powinien odczytać obowiązkowy parametr z nazwą katalogu i używać wskazanego katalogu jako miejsca, z którego rozpocznie przeszukiwanie wskazywanych przez klientów zasobów. Nasz serwer traktuje wszystkie identyfikatory zasobów
    wskazywane przez klientów jako nazwy plików we wskazanym katalogu. Nasz serwer powinien odnajdywać w trakcie swojego działania także pliki, które zostały dodane/usunięte/zmodyfikowane po chwili uruchomienia serwera.</p>
<p>Serwer może założyć, że żądania klientów o zasoby powinny zawsze rozpoczynać się od znaku “/”, który jednocześnie wskazuje nam iż rozpoczynamy poszukiwania pliku od korzenia jakim jest katalog przekazany w parametrze podczas uruchomienia naszego serwera.
    Jeśli zapytanie klienta nie spełnia tego założenie, to serwer powinien odpowiedzieć błędem o numerze 400.</p>
<p>Serwer powinien traktować kolejne wystąpienia znaku “/”, w identyfikatorze zasobu (<tt>request-target</tt>), jako znak oddzielający poszczególne nazwy katalogów w ścieżce do pliku. Serwer powinien próbować odnaleźć plik zgodnie ze ścieżką otrzymaną od
    klienta w identyfikatorze zasobu. W przypadku gdy plik nie zostanie odnaleziony, bądź plik nie jest plikiem zwykłym, do którego serwer ma uprawnienia odczytu, to serwer powinien zachować się zgodnie z wymaganiem opisanym w dalszej części treści zadania
    (
    <a href="#skorelowane">Skorelowane serwery HTTP</a>).</p>
<h2 id="h.bht8r9fkt15r">Obsługa błędów</h2>
<p>Nasz serwer powinien w miarę możliwości zawsze wysłać komunikat zwrotny do klienta. Komunikat powinien informować klienta o statusie przetworzenia&nbsp;jego żądania. Sewer powinien zwracać następujące kody (<tt>status-code</tt>):</p>
<ul>
    <li>200 - ten kod informuje klienta, że jego żądanie zostało w pełni i poprawnie wykonane przez nasz serwer. Ten kod używamy w szczególności kiedy przesyłamy do klienta zawartość szukanego przez klienta pliku.</li>
    <li>302 - ten kod oznacza, że szukany przez klienta zasób został tymczasowo przeniesiony pod inną lokalizację. Tego kodu użyjemy do implementacji nietypowego rozszerzenia naszego serwera, opisanym w dalszej części treści zadania (<a href="#skorelowane">Skorelowane serwery HTTP</a>).</li>
    <li>400&nbsp;- ten kod nasz serwer powinien wysłać zawsze, i tylko w przypadku, kiedy żądanie serwera nie spełnia oczekiwanego formatu, bądź zawiera elementy, które treść zadania wykluczyła jako akceptowane. </li>
    <li>404 - ten kod informuje klienta, że żądany przez niego zasób nie został odnaleziony. Nasz serwer powinien wysyłać ten błąd w przypadku nieodnalezienia żądanego przez klienta pliku - z uwzględnieniem jednak dodatkowej logiki opisanej w dalszej części
        treści zadania (<a href="#skorelowane">Skorelowane serwery HTTP</a>).</li>
    <li>500 - ten kod reprezentuje błąd po stronie naszego serwera w przetwarzaniu żądania klienta. Błąd ten serwer może, i powinien, wysłać do klienta w przypadku problemów występujących po stronie z serwera nie wynikających z błędu po stronie klienta. Ten
        kod błędu oznacza generyczny błąd pracy serwera.
    </li>
    <li>501 - ten kod błędu serwer może zwrócić w przypadku żądań klienta wykraczających poza skromny zakres implementacji naszego serwera. Dla przykładu wszystkie żądania z metodami różnymi od <tt>GET</tt> i <tt>HEAD</tt>. Kod ten informuje klienta, że serwer
        nie zaimplementował obsługi otrzymanego żądania.
    </li>
</ul>
<p></p>
<p>Zwracamy uwagę, iż opisane powyżej kody błędów stanowią jedynie wycinek zdefiniowanych błędów standardu HTTP oraz nie pokrywają się dokładnie z poprawną semantyką błędów HTTP (dla przykładu nasz serwer w przypadku błędów 4xx nie zwraca żadnego ciała odpowiedzi,
    co nie jest zgodne z RFC7231). Zabieg ten pozwolił uprościć implementację zadania, ale student powinien pamiętać, iż niniejsze zadanie w żadnej mierze nie stanowi choćby wycinka referencyjnej implementacji poprawnego serwera protokołu HTTP. Podczas
    implementacji niniejszego zadania należy postępować zgodnie z zaprezentowaną treścią, a w przypadku wątpliwości zadać pytanie prowadzącym przedmiot. W przyszłości w celu poprawnej implementacji serwera HTTP należy postępować zgodnie z dokumentacją
    RFC odpowiednią dla implementowanej wersji protokołu HTTP.</p>
<h2 id="h.4jk6c6xh3jzj">Obsługiwane nagłówki</h2>
<p>Nasz serwer powinien obsługiwać co najmniej nagłówki o następujących wartościach <tt>field-name</tt>:</p>
<ul>
    <li><tt>Connection</tt>&nbsp;- domyślnie połączenie nawiązane przez klienta powinno pozostać otwarte tak długo jak klient nie rozłączy się albo nasz serwer nie uzna danego klienta za bezczynnego i nie zakończy połączenia. Klient w ramach jednego połączenie
        może przesłać do serwera więcej niż jeden komunikat i oczekiwać odpowiedzi serwera na każdy z wysłanych komunikatów. Odpowiedzi serwera powinny następować w kolejności odpowiadającej przychodzącym żądaniom klienta. Nagłówek <tt>Connection</tt>&nbsp;ustawiony
        z wartością <tt>close</tt>&nbsp;pozwala zakończyć połączenie TCP po komunikacie odpowiedzi wysłanej przez serwer, następującej po komunikacie zawierającym wspomniany nagłówek ze wskazaną wartością. Zatem jeśli klient wyśle komunikat żądania z
        nagłówkiem
        <tt>Connection: close</tt>, to serwer powinien zakończyć połączenie po wysłaniu komunikatu odpowiedzi. Serwer sam również może zakończyć połączenie, dodając nagłówek <tt>Connection: close</tt>&nbsp;do swojej odpowiedzi, choć nasz serwer nie powinien
        się tak zachowywać bez uzasadnionego powodu.</li>
    <li><tt>Content-Type</tt> - jest nagłówkiem opisującym jakiego rodzaju dane przesyłamy w treści (ciele) komunikatu HTTP. Dzięki wartości tego nagłówka serwer może poinformować klienta czy otrzymany przez niego zasób jest obrazem, tekstem, stroną html,
        plikiem PDF, etc. Implementacja zadania może zawsze określać wysyłane pliki z serwera jako strumień bajtów <tt>application/octet-stream</tt>.</li>
    <li><tt>Content-Length</tt> - wartość tego nagłówka, wyrażona nieujemną liczbą całkowitą, określa długość treści (ciała) komunikatu HTTP. Wartość tego nagłówka jest wymagana w każdej odpowiedzi z serwera, która zawiera treść (ciało). Wartość tego nagłówka
        określa wyłącznie liczbę oktetów treści (ciała) komunikatu HTTP. Serwer powinien obsługiwać omawiany nagłówek także w komunikatach wysyłanych przez klienta.</li>
    <li><tt>Server</tt> - jest to parametr opcjonalny jaki serwer może umieszczać w każdym komunikacie wysyłanym&nbsp;do klienta. Wartością tego nagłówka może być dowolny napis indentyfikujący&nbsp;implementację serwera, jego nazwę.</li>
</ul>
<p></p>
<p>Nagłówki nie wymienione powyżej, a otrzymane w komunikacie od klienta ignorujemy.</p>
<p>W przypadku wystąpienia więcej niż jednej linii nagłówka o tej samej wartości <tt>field-name</tt>, serwer powinien potraktować takie żądanie jako niepoprawne i odpowiedzieć statusem o numerze 400.</p>
<h2 id="h.d5sbstfyla9t">Obsługiwane metody</h2>
<p>Nasz serwer powinien obsługiwać minimum dwie następujące metody:</p>
<ul>
    <li><tt>GET</tt> - w przypadku otrzymania żądania od klienta z tą metodą, serwer powinien podjąć próbę odnalezienia wskazanego zasobu (pliku) w katalogu jaki został przekazany jako parametr przy uruchomieniu programu. Jeśli plik zostanie odnaleziony,
        to serwer powinien zwrócić zawartość tego pliku poprzez wysłanie odpowiedniego komunikatu HTTP z treścią (ciałem) uzupełnionym oktetami odpowiadającymi bajtom odczytanym z pliku. Serwer powinien ustawić typ zwracanego pliku w nagłówku <tt>Content-Type</tt>.
        Dla uproszczenia implementacji serwer może użyć wartości <tt>application/octet-stream</tt>&nbsp;dla dowolnego pliku.<br>Jeśli plik nie zostanie odnaleziony na serwerze, to serwer powinien zachować się zgodnie z wymaganiem opisanym w dalszej części
        treści (
        <a href="#skorelowane">Skorelowane serwery HTTP</a>).</li>
    <li><tt>HEAD</tt> - w przypadku otrzymania żądania z tą metodą, serwer powinien odpowiedzieć dokładnie takim samym komunikatem jak gdyby otrzymał żądanie z metodą <tt>GET</tt>, z tą różnicą, że serwer przesyła komunikat bez treści (ciała). Odpowiedź serwera
        na żądanie
        <tt>HEAD</tt> powinna być taka sama jaką otrzymałby klient wykonując metodę <tt>GET</tt> na wskazanym zasobie, w szczególności nagłówki także powinny zostać zwrócone przez serwer takie same jak w przypadku metody <tt>GET</tt>.</li>
</ul>
<h2 id="h.yrrzy2ec6ne7"><a name="skorelowane"></a>Skorelowane serwery HTTP</h2>
<p>Implementacja serwera powinna przyjmować obowiązkowy parametr wejściowy wskazujący (ścieżką bezwzględną bądź względną) na plik tekstowy&nbsp;o strukturze:</p>
<p></p>
<p><tt>zasób TAB serwer TAB port</tt></p>
<p>gdzie:</p>
<ul>
    <li><tt>zasób</tt>&nbsp;- to bezwzględna ścieżka do pliku.</li>
    <li><tt>TAB</tt>&nbsp;- znak tabulacji.</li>
    <li><tt>serwer</tt>&nbsp;- to adres IP serwera, na którym wskazany zasób się znajduje. Adres jest adresem IP w wersji 4.</li>
    <li><tt>port</tt>&nbsp;- to numer portu, na którym serwer nasłuchuje na połączenia.</li>
</ul>
<p></p>
<p>Serwer powinien odszukać plik na podstawie ścieżki bezwzględnej bądź ścieżki względnej w aktualnym katalogu roboczym. Jeśli plik nie zostanie odnaleziony, bądź nie możliwe było jego odczytanie, to serwer powinien zakończyć swoje działanie z kodem błędu
    EXIT_FAILURE. Implementacja serwera może założyć, że wczytywany plik posiada poprawną strukturę, zgodną z zadaną w niniejszej treści zadania, a plik pusty także jest poprawnym plikiem. Implementacja serwera powinna odczytać zawartość pliku i w przypadku
    otrzymania od klienta żądania HTTP dotyczącego zasobu, którego serwer nie znalazł w plikach zlokalizowanych lokalnie, powinien przeszukać wczytaną z pliku tablicę i odszukać żądany przez klienta zasób. Jeśli zasób nie występuje także we wczytanej
    tablicy, to serwer powinien odpowiedzieć statusem o numerze 404 do klienta. Jeśli jednak zasób znajduje się na liście, to serwer powinien&nbsp;wysłać odpowiedź do klienta ze statusem numer 302 oraz ustawionym nagłówkiem <tt>Location</tt>, którego
    wartość powinna być tekstem reprezentującym adres HTTP do serwera zawierającego szukany zasób. Jeśli szukany zasób występuje więcej niż raz w tablicy wczytanej z pliku, to serwer powinien skorzystać z pierwszego wpisu występującego najwcześniej w
    pliku. Konstruowanie nowego adres szukanego zasobu należy wykonać następującego:
</p>
<p><tt>PROT serwer COLON port zasób</tt></p>
<p>gdzie:</p>
<ul>
    <li><tt>PROT</tt>&nbsp;- to napis <tt>http://</tt>.</li>
    <li><tt>serwer</tt>&nbsp;- to adres IP serwera, odczytany z pliku.</li>
    <li><tt>COLON</tt>&nbsp;- to znak dwukropka.</li>
    <li><tt>port</tt>&nbsp;- to numer portu, odczytany z pliku.</li>
    <li><tt>zasób</tt>&nbsp;- to bezwzględna ścieżka do zasobu (zaczynająca się od znaku slash <tt>/</tt>).</li>
</ul>
<p></p>
<h2 id="h.wa9514t4ng3g">Dodatkowe wymagania</h2>
<p>Katalog z rozwiązaniem powinien zawierać pliki źródłowe oraz plik Makefile zapewniający automatyczną kompilację i linkowanie. Po wywołaniu komendy "make", na serwerze students, program musi się poprawnie kompilować. Do komunikacji sieciowej należy używać
    wyłącznie omawianego na zajęciach mechanizmu gniazd z biblioteki standardowej.</p>
<h3 id="h.yaz3vdjrdw3e">Format rozwiązania:</h3>
<p>Jako rozwiązanie należy wysłać poprzez moodle plik ab123456.tar.gz, gdzie ab123456 to login na students. &nbsp;W wysłanym powinien znaleźć się katalog ab123456 zawierający pliki źródłowe i plik Makefile.</p>
<p>We wspomnianym katalogu proszę nie umieszczać plików binarnych ani żadnych plików pośrednich powstających podczas kompilacji. &nbsp;W wyniku wykonania polecenia make powinien powstać plik wykonywalny: serwer.</p>
<p>Ponadto Makefile powinien obsługiwać cel (target) 'clean', który po wywołaniu kasuje wszystkie pliki powstałe podczas kompilacji.</p>
