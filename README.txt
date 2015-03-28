This was my fourth homework for a computer graphics course in the 3rd year.

Task: implementing a 3D scene in C++ using OpenGL with a huge number of textured
objects illuminated with a spot light and applying the View Frustum Culling
algorithm for that scene.

Full statement:
http://cs.curs.pub.ro/2014/pluginfile.php/10729/mod_resource/content/1/tema_culling.pdf
(You might not have access to it as a guest.)

Check out the repo wiki for some screenshots.

Original README file:
================================================================================
EGC - Tema 4
    View frustum culling
    Mirea Stefan-Gabriel, 331CC

Cuprins

1. Cerinta
2. Utilizare
3. Implementare
4. Testare
5. Probleme Aparute
6. Continutul Arhivei
7. Functionalitati

1. Cerinta
--------------------------------------------------------------------------------
Tema presupune implementarea unei scene cu un numar mare de obiecte, texturate
si iluminate cu o lumina de tip spot (per fragment), pentru care se va
implementa algoritmul view frustum culling. Exista doua unghiuri de vizualizare:
view1, camera ce se deplaseaza odata cu lumina de tip spot si cu un obiect
reprezentand vehiculul prin scena si view2, care vizualizeaza scena de la o
inaltime mare si care reprezinta cu rosu obiectele invizibile din primul unghi
si cu verde pe cele vizibile.

2. Utilizare
--------------------------------------------------------------------------------
2.1 Consola

Programul va crea o scena cu un numar de case dispuse sub forma unui patrat. In
linia de comanda se poate da un argument reprezentand numarul de case de-a
lungul unei laturi, care trebuie sa fie par si mai mare sau egal cu 6. Daca
argumentul lipseste, valoarea implicita va fi 16 (constanta simbolica
DEFAULT_ARRAY_DIM), rezultand un total de 256 de case. Exemplu de apel:
C:\331CC_Mirea_Stefan_Tema4EGC>.\Debug\egc_tema4.exe 10

2.2 Input Tastatura

- Esc: Inchide glut;
- Space: Pauza (inclusiv in filmul de la inceput);
- Enter: Sarire peste filmul de la inceput;
- 'n': Restart (doar dupa terminarea filmului);
- '1': Activeaza camera view1;
- '2': Activeaza camera view2;
- 'w', sageata sus: Deplasare in fata;
- 's', sageata jos: Deplasare inapoi;
- 'a': Deplasare la stanga;
- 'd': Deplasare la dreapta;
- 'r': Deplasare in sus;
- 'f': Deplasare in jos;
- 'q', sageata stanga: Rotatie pe Oy la stanga;
- 'e', sageata dreapta: Rotatie pe Oy la dreapta;
- 't': Rotatie pe Ox in sus;
- 'g': Rotatie pe Ox in jos.

3. Implementare
--------------------------------------------------------------------------------
Platforma:
   IDE: Microsoft Visual Studio Professional 2013
   Compilator: Microsoft C/C++ Optimizing Compiler Version 18.00.21005.1 for x86
   Sistem de operare: Microsoft Windows 7 Ultimate SP1 Version 6.1.7601
   Placa video: ATI Mobility Radeon HD 4570 512MB VRAM

Generalitati:
Avand in vedere necesitarea folosirii unui numar relativ mic (18) de tipuri de
mesh-uri (casa fara etaje, casa cu etaj, strada continua etc.) si a unui numar
mare de instante ale acestora - majoritatea stocate intr-un tablou bidimensional
oricat de mare, probabil s-ar fi putut considera ca cea mai adecvata varianta,
din punct de vedere al programarii orientate pe obiecte, ar fi fost folosirea
unei clase de baza care sa retina anumite campuri generice (precum matricea de
modelare) si a cate unei clase derivate din aceasta pentru fiecare tip de
obiect. Mai departe, cum toate mesh-urile de un anumit tip sunt identice, s-ar
fi preferat ca geometria (vbo, ibo, vao, num_indices, vectorul de vertecsi -
retinut separat pentru aplicarea algoritmului de culling) sa fie stocata sub
forma unor campuri statice ale claselor derivate, si, pentru a putea accesa
aceste campuri pentru un oarecare obiect despre care nu se cunoaste ce clasa
derivata instantiaza, ar fi trebuit create gettere virtuale. Pe de alta parte,
pentru a putea crea geometria inaintea crearii instantelor sau a o distruge dupa
distrugerea instantelor, ar fi fost necesare si niste gettere statice (apelate
din metode statice ale clasei de baza), deoarece o metoda nu poate fi simultan
si virtuala si statica. Asta ar fi dus la un numar foarte mare de clase, fiecare
cu metode similare, in ambele forme (virtuale si statice), ceea ce nu ar fi fost
prea elegant. Prin urmare, am recurs la o cu totul alta metoda: am creat o clasa
(MeshType) ce descrie un tip de mesh si un vector (mesh_types) cu toate
instantele acesteia, si o clasa (Mesh) ce descrie un mesh individual (avand
inclusiv un camp - mesh_type pentru identificarea tipului in cadrul vectorului
mesh_types). Instantele celei din urma sunt:
 - elementele matricei world, reprezentand casele, spatiile verzi dintre ele si
strazile;
 - elementele vectorilor margins si corners, reprezentand niste spatii patratice
plasate in jurul obiectelor din world, care fac legatura cu suprafetele
redimensionabile;
 - nava mama (mothership).
Personajul principal nu a putut extinde tot clasa Mesh din urmatorul motiv: in
timpul deplasarii sale, acesta se inclina pentru a simula producerea propulsiei.
Intrucat am ales sa nu folosesc rotiri explicite ale camerei view1, ci tastele
sa controleze direct personajul si sa pozitionez la fiecare cadru camera view1
relativ la acesta, am avut nevoie sa retin matricea de modelare separata de
inclinari, care nu trebuie sa afecteze pozitia camerei, insa de ele trebuie sa
se tina cont la desenare, testarea coliziunilor si la calculul pozitiei sursei
luminii de tip spot. Astfel, am creat clasa Character ce extinde Mesh, oferind
in plus suport pentru inclinari si controlul personajului.
De asemenea, pentru retinerea suprafetelor ce se redimensioneaza pentru a parea
infinite, am creat clasa InfSurface, ce extinde direct MeshType (fiecare
suprafata are propria sa geometrie, precum tipurile de mesh-uri). Acestea sunt
opt la numar (retinute in vectorul infinite_surfaces) si sunt amplasate
imprejurul marginilor si colturilor. Fiecare retine un nivel de extindere pentru
axa / axele corespunzatoare, ce se actualizeaza in notifyBeginFrame() in functie
de pozitia camerei.
Pentru implementarea algoritmului view frustum culling, am adaugat camerei un
camp ce retine frustumul prin ecuatiile implicite ale planelor in spatiul
utilizator si o metoda ce il actualizeaza (updateFrustum()) pe baza matricei de
proiectie, care se apeleaza inainte de desenare. Calculul coeficientilor
ecuatiilor pleaca de la urmatoarea proprietate descrisa in acest document[1],
pag. 4: dupa inmultirea la stanga a unui vector din spatiul coordonatelor de
vizualizare cu matricea de proiectie, punctul rezultat va apartine interiorului
frustumului daca si numai daca primele trei componente sunt incluse in
intervalul centrat in origine, de lungime data de cea de-a patra componenta. In
cazul meu, transformarea trebuia sa se aplice asupra unui punct din world space,
asa ca la mine matricea o reprezenta produsul dintre matricele de proiectie
respectiv vizualizare (in document e o greseala in sectiunea 3.1, punctul 2:
matricea V trebuia inmultita la dreapta), iar formulele rezultate sunt negate,
pentru a avea normalele spre exterior. Deoarece obiectele asupra carora se
aplica algoritmul nu se deplaseaza, am adaugat in clasa Mesh un camp pentru
coordonatele in world space, care raman constante, iar la fiecare desenare le
trimit catre metoda isVisible() a clasei Frustum, ce implementeaza algoritmul
descris in enunt si spune daca obiectul trebuie desenat sau nu. Deoarece in
cazul meu si pamantul e alcatuit dintr-un numar mare de obiecte (dreptunghiuri),
am aplicat culling si pentru acestea, iar view2 le reprezinta colorandu-le mai
inchis.
Pentru implementarea luminii de tip spot, am modificat componenta difuza, care
este proportionala cu (cos(alpha) - cos(cut-off)) / (1 - cos(cut-off)), unde
alpha este unghiul dintre V (vectorul de la lumina la punctul curent) si D
(directia spotului) si am implementat si atenuarea, cu un factor proportional cu
inversul patratului distantei dintre punct si sursa de lumina. Combinarea dintre
lumina si culoarea texturii am facut-o impunand ca rezultatul sa fie o functie
liniara pe portiuni de lumina. Astfel, independent pentru fiecare canal:
 - daca lumina este 0, culoarea va fi 0;
 - daca lumina este 0.5, culoarea finala va fi culoarea texturii;
 - daca lumina este 1, culoarea finala va fi 1.
Functia applyLight() din fragment shader realizeaza interpolarea liniara intre
aceste valori.
Animatiile initiale sunt implementate prin stocarea (in clasa Tema) a unui camp
ce retine scena curenta (current_scene). In metoda notifyBeginFrame(), se
testeaza pentru inceput daca timpul scenei curente s-a scurs, trecandu-se in caz
afirmativ la urmatoarea scena, cu eventuale initializari. Apoi, in functie de
scena curenta si de timpul trecut in cadrul ei, se actualizeaza pozitiile
camerei si ale obiectelor pe baza a diferite tipuri de functii (de gradul I, II,
sau chiar radical), in functie de modul in care am dorit sa arate traiectoriile.

Mentiuni:
Ca si la tema 3, pentru a tine evidenta tuturor tastelor apasate la un
momentdat, folosesc doua bitseturi:
  - pressed_keys retine pentru fiecare tasta 1 daca este apasata sau 0 in caz
    contrar;
  - is_special retine pentru fiecare tasta (daca e apasata) 1 daca e o tasta
    speciala sau 0 in caz contrar.
Astfel, multe din prelucrarile corespunzatoare tastelor sunt efectuate in
notifyBeginFrame(), verificand daca tastele sunt apasate in acel moment prin
apeluri ale metodei isPressed(). Rezultatul este o miscare mai cursiva,
deoarece nu mai apare delay-ul care cere confirmarea mentinerii tastelor
apasate. Prin urmare, efectele vor depinde cu precizie de timpul apasarii.
Am renuntat la rotirea pe Oz a personajului / camerei, deoarece nu este foarte
utila (nici in majoritatea jocurilor nu exista). De asemenea, rotirile fata de
Oy se realizeaza in spatiul utilizator, pentru a mentine intotdeauna vectorul up
al camerei intr-un plan vertical.
Atat pentru case cat si pentru nava mama, am folosit multitexturare. In acest
fel, am redus geometria verandei, respectiv a steagului.
Pentru imbunatatirea performantei am implementat back-face culling, verificand
in fragment shader daca produsul scalar dintre normala si vectorul care uneste
punctul curent de camera este negativ. Acest lucru are si dezavantaje:
garduletul verandei nu se vede din interiorul ei (la fel, steagul navei mama se
vede dintr-o singura parte, dar in film camera se afla doar in partea
corespunzatoare). O exceptie o constituie "usile" de la baza navei mama in
timpul deschiderii lor, cand e importanta si observarea celor din spate.
Deoarece numarul de cadre afisare intr-o secunda nu este consistent, folosesc
performance counters pentru a tine evidenta wall clock time-ului, necesar in
situatii precum calculul variatiilor diferitelor marimi intre doua cadre
consecutive, al clipirii personajului cand view2 e activa, precum si al timpilor
alocati scenelor din cadrul animatiei initiale.

4. Testare
--------------------------------------------------------------------------------
Tema a fost testata doar pe calculatorul personal (platforma mentionata la 3.).
Am incercat sa acopar toate cazurile posibile, in materie de valori ale
numarului de case si ale constantelor simbolice din main.cpp (in limita
restrictiilor din comentarii, datorate modului in care am gandit semnificatia
valorilor respective) si mi-am asigurat ca executia programului, precum si
animatiile de la inceput, se realizeaza corect. De exemplu, determinarea unei
strazi apropiate centrul scenei, relativ la care sunt descrise animatiile si
pozitia initiala a personajului, implementata in metoda findCenter(), se face
diferit in functie de restul impartirii numarului de case de-a lungul unei
laturi la 8. De asemenea, toate traiectoriile camerei sau ale obiectelor in
cadrul filmului sunt descrise in functie de valori precum CELL_SIZE - lungimea
unei case, GAP_SIZE - distanta pe Oz intre doua case alaturate, ducand la o
scena foarte configurabila.

5. Probleme aparute
--------------------------------------------------------------------------------
Au fost mai multe probleme intampinate. De exemplu, datorita modului in care
functioneaza filtrarea triliniara, la incercarea de a selecta o anumita zona
dreptunghiulara dintr-o imagine, culorile de pe marginea fetei texturate vor fi
influentate de texeli adiacenti care nu apartin zonei, ducand la probleme de
continuitate intre fete lipite. Mai exact, sa presupunem ca la dreapta unui
patrat texturat cu imaginea resurse\margin.bmp trebuie lipit un alt patrat,
texturat cu resurse\corner.bmp. Daca pentru varfurile patratului din dreapta se
vor alege coordonatele de textura (0, 0), (0, 1), (1, 0), (1, 1), atunci
culorile celor mai din stanga pixeli ai sai vor fi apropiate de culorile celor
mai din dreapta, datorita tendintei texturii de a se repeta pe suprafata
patratului, ducand la o discontinuitate a culorilor intre cele doua patrate. Din
acest motiv, m-am asigurat ca, in cadrul imaginilor, fiecare triunghi folosit
efectiv pentru texturarea unei fete este distantat de marginea imaginii si
bordat corespunzator.

Tot aici ar trebui precizata o deficienta a algoritmului descris in enunt: daca
un obiect se afla simultan in mai multe parti relativ la frustum (nu doar in
dreapta sa sau doar in stanga etc.), niciunul din cele sase plane nu va reusi sa
il separe in exterior in intregime, asa ca obiectul inca va fi desenat. Acest
efect se poate observa uneori pentru dreptunghiurile de pe margine, deoarece ele
sunt suficient de lungi.

6. Continutul Arhivei
--------------------------------------------------------------------------------
resurse\*.bmp - imagini pentru texturare
resurse\*.obj - grafica celor doua tipuri de case, modelate de mine in Google
                SketchUp
shadere\shader_fragment.glsl - fragment shader-ul
shadere\shader_vertex.glsl - vertex shader-ul
lab_camera.hpp - implementarea camerei - modificat prin adaugarea ca membru
                 privat a unui obiect ce gestioneaza frustumul si implementeaza
                 algoritmul view frustum culling, a unor metode pentru
                 actualizarea si interogarea sa si a unei metode ce intoarce
                 pozitia camerei, pentru a nu o retine si in clasa Tema din
                 main.cpp
lab_mesh_loader.hpp - incarcarea unui mesh din fisier - modificat pentru ca
                      lab::VertexFormat sa poata primi la constructor coordonate
                      polare pentru pozitie si normala in planul xOz, forma
                      utila la constructia navelor
main.cpp - sursa principala
Toate celelalte fisiere fac parte din scheletul din laborator.

7. Functionalitati
--------------------------------------------------------------------------------
7.1 Functionalitati standard (toate cele din enunt)

    - lumina de tip spot;
    - scena cu multe obiecte texturate;
    - view frustum culling corect;
    - camera, vehicul.

7.2 Functionalitati Bonus / Suplimentare

    - Toate obiectele (case, nave, strazi, suprafete marginale) au fost modelate
de mine (geometrie, normale, coordonate de textura). Pentru realizarea
texturilor am folosit si imagini din alte surse[2][3][4][5][6][7][8][9][10].
    - Scena este organizata dupa un pattern complex, conform caruia, pe axa Oz,
cate doua siruri de case sunt despartite de strazi, iar pe axa Ox, cate 4
siruri. In plus, intre sirurile de case alaturate pe Ox apar spatii
suplimentare, a caror dimensiune (x_gap) se calculeaza automat pentru ca
intreaga arie sa fie patratica. Pe baza anumitor reguli, toate casele sunt
orientate spre una din strazile adiacente.
    - Toate obiectele sunt texturate incat sa dea senzatia de continuitate.
Astfel, desi strazile sunt alcatuite dintr-un numar mare de dreptunghiuri puse
cap la cap, exista texturi speciale pentru diferite tipuri de intersectii sau
pentru sfarsitul strazii, incat ele sa para continue. De asemenea, drumurile ce
unesc garajele de strazi se intind de-a lungul a doua obiecte: casa si strada,
parand totusi continue.
    - Terenul pare infinit, datorita redimensionarii suprafetelor
dreptunghiulare marginale in functie de pozitia camerei. Pentru ca linia
orizontului sa nu para franta (din cauza marginilor acestor suprafete la un
momentdat), am definit o distanta maxima de vizibilitate (MAX_DIST) si am
simulat un efect de ceata in fragment shader. Totodata, ca optimizare, am
limitat frustumul cand camera view1 e activa la MAX_DIST, pentru ca obiectele
care nu se vad din cauza cetii sa fie si ele eliminate (dar inca vor fi desenate
cu verde cand view2 e activa).
    - Am implementat coliziuni cu pamantul.
    - Lumina de tip spot are o culoare galben-verzuie.
    - Am creat si o lumina solara, sub forma unei lumini directionale: vectorul
spre sursa de lumina (L) este constant.
    - Personajul se inclina pentru a se deplasa pe Ox sau Oz, simuland
producerea propulsiei. Viteza de deplasare la un momentdat e proportionala cu
sinusul unghiului inclinarii. Cum lumina de tip spot se misca odata cu
personajul, aceasta va fi de asemenea afectata de inclinari, ceea ce incalca
precizarea din enunt: "Lumina de tip spot se misca impreuna cu camera normala".
Din acest motiv, am definit in main.cpp constanta simbolica ENABLE_TILT, ce
poate fi setata la false pentru a dezactiva inclinarile.
    - Am realizat un filmulet introductiv. Am introdus si posibilitatea de a
pune pauza, valabila si dupa terminarea filmului.

Referinte
--------------------------------------------------------------------------------
[1] http://web.archive.org/web/20120531231005/http://crazyjoke.free.fr/doc/3D/plane%20extraction.pdf
[2] http://texturelib.com/Textures/grass/grass/grass_grass_0118_02_preview.jpg
[3] http://bgfons.com/upload/tile_texture3063.jpg
[4] http://texturelib.com/Textures/road/road/road_road_0011_01_preview.jpg
[5] http://img.cadnav.com/allimg/131004/1-131004112025315.jpg
[6] http://www.aoaforums.com/forum/attachments/digital-image-photo-video-audio-editing/25268d1275609365-camoflage-seamless-texture-maps-free-use-clay-ground-seamless.jpg
[7] http://texturelib.com/Textures/roof/roof_0052_01_preview.jpg
[8] http://www.commentnation.com/backgrounds/images/brown_bricks_wall_seamless_background_texture.jpg
[9] http://www.georgeandsonsdoors.com/wp-content/uploads/2011/12/Universal_225.jpg
[10] https://www.google.com/maps/@44.4384009,26.0514932,71m/data=!3m1!1e3