# Truco86 en el navegador

Página estática que corre `truco86.nes` con [EmulatorJS](https://emulatorjs.org/)
(frontend web de RetroArch, cargado por CDN — no hay build step ni `node_modules`).
Usa el core `fceumm` (mismo motor que FCEUX, el emulador con el que se probó todo
el juego durante el desarrollo), que corre en WASM de un solo hilo — no necesita
headers especiales de servidor (`Cross-Origin-Opener-Policy`/`Cross-Origin-Embedder-Policy`),
así que un host estático como Vercel alcanza sin configuración extra.

(Antes se probó con [jsnes](https://github.com/jsnes/jsnes), un emulador NES en JS
puro: funcionaba pero con audio entrecortado/con delay y glitches visuales en
esta ROM — EmulatorJS/fceumm los resolvió de raíz al ser un core mucho más
preciso, así que se reemplazó por completo.)

## Qué hay en la página

Arriba el reproductor, y debajo: la tabla de teclas, un resumen de cómo se
juega, y los enlaces al repo de GitHub y a la descarga del `.nes` (el mismo
archivo que carga el emulador, servido con `download`).

La tabla de teclas se muestra **solo en desktop** (`@media (hover: hover) and
(pointer: fine)`): en celular/tablet no hay teclado y EmulatorJS dibuja su
propio pad en pantalla, así que ahí se muestra esa aclaración en vez de las
teclas. Las teclas de la tabla son las que mapea EmulatorJS por defecto
(D-PAD = flechas, A = Z, B = X, START = Enter, SELECT = Espacio); el juego
solo usa el D-PAD, A y START — B y SELECT están listados como "sin uso" para
que nadie los busque.

## Actualizar el `.nes` que se sirve

```sh
make web   # compila (si hace falta) y copia build/truco86.nes -> web/truco86.nes
```

`web/truco86.nes` **sí se versiona en git** (hay una excepción puntual en
`.gitignore` para este archivo, aunque el resto de `*.nes` del proyecto no
se versiona): así el deploy en Vercel es 100% estático, sin necesitar cc65
instalado en el servidor de build.

## Probar local

```sh
cd web
python3 -m http.server 8000
```

Abrir `http://localhost:8000` en el navegador. EmulatorJS trae su propia
pantalla de "play" (los navegadores no dejan reproducir audio sin que el
usuario toque algo primero — no hay forma de saltarse eso) y su propio
menú de configuración de controles (ícono en la esquina del reproductor),
con mapeo de teclado tipo RetroArch por defecto.

## Deploy a Vercel (gratis)

1. Conectar el repo de GitHub a un proyecto nuevo en Vercel.
2. En la configuración del proyecto, **Root Directory** → `web`
   (framework preset: "Other" / ninguno — es HTML estático, sin build
   command).
3. Deploy. Vercel da una URL `*.vercel.app` gratis de entrada.
4. Para el dominio propio (`truco86.code.ar`): en Vercel, agregar el
   dominio custom al proyecto; Vercel muestra el registro DNS que hay que
   crear (normalmente un `CNAME truco86 → cname.vercel-dns.com`) en el
   panel de DNS de `code.ar`.

## Nota sobre timing

El core `fceumm` sí soporta temporización PAL, pero EmulatorJS no expone
un selector de región para NES en su configuración por defecto — corre en
NTSC (60 cuadros/seg) a menos que se fuerce la región desde las opciones
avanzadas de RetroArch dentro del propio menú del reproductor. El juego
está pensado para PAL-N (50 cuadros/seg, ver `docs/ARQUITECTURA.md`), pero
como la lógica cuenta cuadros y no segundos reales, nada se rompe — solo
se siente ~20% más rápido que en un NES/emulador PAL real.
