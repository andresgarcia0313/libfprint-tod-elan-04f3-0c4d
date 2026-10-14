# ELAN 04f3:0c4d fingerprint reader on Linux (libfprint TOD driver)

Languages: English first, then [Español](#español). License: LGPL-2.1-or-later.

Makes the ELAN `04f3:0c4d` fingerprint reader (bcdDevice `0x2410`, Lenovo IdeaPad 5 14IAL7 and similar
2022 Lenovo laptops) work on Linux: enroll from KDE Plasma or GNOME settings, unlock the screen and use
`sudo` with your finger. The password always keeps working.

**Why:** with stock libfprint the built-in `elan` driver claims this reader but enrolling fails with
"Calibration failed" or an unknown error (libfprint issues
[#263](https://gitlab.freedesktop.org/libfprint/libfprint/-/issues/263),
[#364](https://gitlab.freedesktop.org/libfprint/libfprint/-/issues/364),
[#822](https://gitlab.freedesktop.org/libfprint/libfprint/-/issues/822)). This module replaces it.

## 1. Check that you have this reader

    lsusb -d 04f3:0c4d                                   # must list the ELAN device
    lsusb -v -d 04f3:0c4d 2>/dev/null | grep bcdDevice   # must say 24.10

Other revisions are left alone (the module only claims `0x2410`).

## 2. Install (Ubuntu 26.04, Kubuntu, amd64)

Download the `.deb` from [Releases](../../releases/latest) and install it with apt, which also pulls
`libfprint-2-tod1` and the OpenCV runtime libraries:

    sudo apt install ./libfprint-2-tod1-elan0c4d_*_amd64.deb
    fprintd-list $USER        # must name "ELAN 04f3:0c4d (rev 0x2410)"

## 3. Enroll your finger

- **KDE Plasma:** System Settings → Users → your user → "Configure fingerprint authentication…" → Add.
- **GNOME:** Settings → System → Users → Fingerprint Login.
- **Terminal:** `fprintd-enroll`

Touch the sensor about 12 times, lifting the finger and moving it slightly each time. "Finger not
centered" means cover the whole sensor. Prints made with the old `elan` driver must be enrolled again.

## 4. Use it

- **Lock screen:** works right after enrolling on KDE and GNOME.
- **sudo and admin dialogs (optional):** from a clone of this repository,

      sudo install -m 0755 packaging/pam/fprint-gate /usr/local/lib/fprint-gate
      sudo install -m 0644 packaging/pam/fprintd-elan0c4d /usr/share/pam-configs/
      sudo pam-auth-update --enable fprintd-elan0c4d

  Three tries of 10 s, then the password prompt. `fprint-gate` skips the finger for `sudo` without a
  terminal (scripts piping a password) and for the login screen.
- **Login screen:** not supported by SDDM/KDE yet (KDE bug
  [506025](https://bugs.kde.org/show_bug.cgi?id=506025), planned for Plasma Login Manager 6.8).

## 5. If something goes wrong

- Not recognized: try again (about 9 of 10 touches pass); if it keeps failing, delete the print
  (`fprintd-delete $USER`) and enroll again covering the whole sensor in different positions.
- See what happened: `journalctl -u fprintd -b`; every step is logged in the `libfprint-elan0c4d`
  domain. For full detail: `sudo systemctl edit fprintd` and add `Environment=G_MESSAGES_DEBUG=all`
  under `[Service]`.
- Uninstall: `sudo pam-auth-update --remove fprintd-elan0c4d` (if enabled) and
  `sudo apt remove libfprint-2-tod1-elan0c4d`.

## How it works

- USB protocol reimplemented in C for interoperability after studying how the Windows driver drives
  the sensor; no vendor code, firmware or keys are included. The dangerous commands `42 01` (RUNIAP)
  and `40 27` (watchdog reset) are never sent, and a test guards the sources against them.
- Capture recalibrates and re-initializes when the sensor returns zero or saturated frames, with typed
  errors and retries. Enroll takes 12 touches with progress and retry hints.
- Matching: SIFT keypoints plus a RANSAC check (OpenCV, like libfprint's SIGFM). The stored template
  holds keypoint positions and descriptors only, never images. Every C++ exception is caught at the C
  boundary and becomes a logged `GError`.
- Measured on the real sensor: another finger never exceeded 3 consistent matches (threshold 8); the
  enrolled finger was accepted on 9 of 13 fresh touches.

## Build from source

    sudo apt install libfprint-2-tod-dev libopencv-dev meson ninja-build
    meson setup build && ninja -C build && meson test -C build
    packaging/build-deb.sh          # release build, runs the tests, writes build-deb/*.deb

The tests use a scripted sensor and synthetic data (no biometric data) and pass with
`-Db_sanitize=address,undefined`. Feedback and tests on other 04f3:0c4d units are welcome.

---

# Español

Idioma: es-CO

**Guía rápida:** comprobar el lector (`lsusb -v -d 04f3:0c4d | grep bcdDevice` debe decir 24.10),
instalar el `.deb` de Releases con `sudo apt install ./libfprint-2-tod1-elan0c4d_*_amd64.deb`, y
registrar el dedo en Preferencias del sistema → Usuarios → "Configurar la autenticación por huella
dactilar…" → Añadir. El detalle de cada paso está abajo.

Módulo externo (TOD) de libfprint para el lector de huellas ELAN 04f3:0c4d, revisión bcdDevice 0x2410.
Es la capa de protocolo portada a C desde la referencia en Python, más un comparador de puntos clave
(SIFT y RANSAC con OpenCV, la misma técnica que el SIGFM de libfprint para sensores pequeños).

## Estado

- Compila con `-Wall -Werror` y exporta `fpi_tod_shared_driver_get_type` (`nm -D` lo muestra).
- Es un `FpDevice` (no `FpImageDevice`): el sensor es de 150x52 y bozorth no sirve.
- `usb_discover` devuelve 100 solo si `g_usb_device_get_release()` es 0x2410; en otra revisión devuelve 0.
- Probado contra el lector real: un enroll de 12 toques completo, sin errores ni reinicios USB. Las pruebas
  automáticas usan un sensor simulado y texturas sintéticas, nunca huellas reales.
- El enroll pide 12 toques, como un celular, y reporta cada uno a fprintd (KDE muestra el avance). Un toque
  con cobertura menor de 0,8 se reporta como `FP_DEVICE_RETRY_CENTER_FINGER` y uno igual al anterior (el dedo
  no se movió) como `FP_DEVICE_RETRY_REMOVE_FINGER`; ninguno de los dos cuenta. Entre toques espera a que el
  dedo se levante, sin plazo propio: termina cuando fprintd cancela.
- Al final el enroll entrega los 12 toques al comparador, que guarda de cada uno sus puntos clave y
  descriptores. Verify e identify capturan un dedo y lo aceptan si al menos 8 coincidencias concuerdan en una
  sola posición con algún toque guardado (`ELAN_MATCH_MIN_INLIERS`).
- Medido en el lector real (25 toques de un dedo y 10 de otro): el otro dedo nunca pasó de 3 coincidencias;
  el dedo propio fue aceptado en 9 de 13 toques nuevos. Un toque rechazado se repite, como en un celular.

## Instalar (Ubuntu 26.04 y Kubuntu)

1. Descargar `libfprint-2-tod1-elan0c4d_<versión>_amd64.deb` de la sección Releases.
2. Instalarlo: `sudo apt install ./libfprint-2-tod1-elan0c4d_*_amd64.deb`
3. Comprobar: `fprintd-list $USER` debe nombrar "ELAN 04f3:0c4d (rev 0x2410)".
4. Registrar el dedo: Preferencias del sistema → Usuarios → "Configurar la autenticación por huella
   dactilar…" → Añadir (en GNOME: Configuración → Usuarios → Inicio de sesión con huella).
5. El bloqueo de pantalla de KDE ya acepta la huella. Para `sudo` y las ventanas de administrador, opcional:

       sudo install -m 0755 packaging/pam/fprint-gate /usr/local/lib/fprint-gate
       sudo install -m 0644 packaging/pam/fprintd-elan0c4d /usr/share/pam-configs/
       sudo pam-auth-update --enable fprintd-elan0c4d

   El filtro `fprint-gate` no pide huella a `sudo` sin terminal (guiones que pasan la contraseña por
   tubería) ni en SDDM. La contraseña siempre sigue sirviendo. Quitar: `sudo pam-auth-update --remove
   fprintd-elan0c4d` y `sudo apt remove libfprint-2-tod1-elan0c4d`.

Las huellas registradas con el driver `elan` de fábrica no sirven con este: hay que registrarlas de nuevo.

## Si algo falla

- No te reconoce: vuelve a tocar (pasan unas 9 de cada 10 veces). Si sigue fallando, borra la huella
  (`fprintd-delete $USER`) y regístrala de nuevo cubriendo todo el lector en posiciones distintas.
- Ver qué pasó: `journalctl -u fprintd -b`; cada paso queda en el dominio `libfprint-elan0c4d`. Para el
  detalle completo: `sudo systemctl edit fprintd` y añadir `Environment=G_MESSAGES_DEBUG=all` en
  `[Service]`.
- La pantalla de inicio (SDDM) todavía no admite huella en KDE (bug
  [506025](https://bugs.kde.org/show_bug.cgi?id=506025), previsto en Plasma Login Manager 6.8).

## Compilar y probar

    sudo apt install libfprint-2-tod-dev libopencv-dev
    meson setup build
    ninja -C build
    meson test -C build

El paquete se genera con `packaging/build-deb.sh` (compila en modo release, pasa las pruebas y calcula las
dependencias). Para probar sin instalar: `FP_TOD_DRIVERS_DIR=<carpeta con el .so>`.

## Qué hace

| Parte | Fichero | Equivale en Python |
|---|---|---|
| Comandos, escritura y lectura USB | `elan0c4d-cmd.c`, `elan0c4d-script.c` | `elan_transport.py`, `elan0c4d.py` |
| Inicio y calibración | `elan0c4d-init.c`, `elan0c4d-calibrate.c` | `initialize`, `calibrate` |
| Imagen cruda (lecturas parciales, máx. 20) | `elan0c4d-image.c` | `image` |
| Preparar captura (fondo) | `elan0c4d-prepare.c` | `prepare_capture` |
| Esperas 40 3e / 40 3f | `elan0c4d-wait.c`, `elan0c4d-codes.c` | `elan_wait.py` |
| Armado del detector | `elan0c4d-arm.c` | `_arm` |
| Trama: recalibrar y reinicializar | `elan0c4d-frame.c` | `read_frame` |
| Flujo de captura | `elan0c4d-capture.c` | `capture_finger` |
| Esperar que el dedo se levante | `elan0c4d-lift.c` | `wait_lift` |
| Enroll por toques y reintentos | `elan0c4d-stages.c`, `elan0c4d-enroll.c` | bucle de `elan0c4d_test.py` |
| Imagen de 8 bits y parecido entre toques | `elan0c4d-normalize.c` | `normalized` |
| Datos guardados en el `FpPrint` | `elan0c4d-print.c` | - |
| Reintentos y remedios | `elan0c4d-retry.c`, `elan0c4d-recover.c`, `elan0c4d-try.c` | `elan_session.py` |
| Errores | `elan0c4d-error.c` | `elan_errors.py` |
| Clasificador EMPTY, FINGER, INVALID | `elan0c4d-quality.c` | `elan_quality.py` |
| Comparador (interfaz C y errores) | `elan0c4d-match.c` | - |
| SIFT, RANSAC y plantilla (C++, OpenCV) | `elan0c4d-sift*.cpp` | - |

Cada operación es una máquina de estados (`FpiSsm`) sobre `fpi_usb_transfer`. En C no hay excepciones: cada paso
termina con un `GError` que el nivel superior decide si reintenta (con su remedio) o propaga. Las
cancelaciones y los dispositivos desaparecidos nunca se reintentan. Ante 0xFF en una espera se reenvía el
comando; no se sigue leyendo en EP 0x83. Un tiempo agotado o una cancelación en la espera envían 00 0b.

El comparador en C++ atrapa toda excepción (`cv::Exception`, `std::bad_alloc`, cualquier otra) en su frontera
(`elan0c4d-sift-api.cpp`) y la devuelve como estado y mensaje; el driver la convierte en `GError` y la registra.

El `FpPrint` guarda en `fpi-data` un `GVariant` `(qqqqay)`: versión de formato (2), ancho, alto, número de toques
y la plantilla. La plantilla (`E0S1`) lleva solo posiciones de puntos clave y descriptores SIFT, nunca imágenes
del dedo. Un print de otra versión, geometría o plantilla dañada se rechaza con `FP_DEVICE_ERROR_DATA_INVALID`.

## Reglas de seguridad

- 42 01 (RUNIAP) y 40 27 (WDTRST) no existen en el código. Si algo los construyera,
  `elan_cmd_is_forbidden` los rechaza antes de escribir y la prueba `no-forbidden-bytes` vigila las fuentes.
- No hay escrituras de registros ni nada del controlador de Windows.

## Registro

Todo sale por GLib con el dominio `libfprint-elan0c4d`: `g_debug` cada comando y respuesta (hex),
`g_info` hitos (versión, calibración, trama), `g_warning` cada fallo y recuperación.
Ejemplo: `G_MESSAGES_DEBUG=all fprintd`.

## Pruebas

Clasificador con tramas sintéticas (vacío, crestas, ceros, saturada 0x3FFE, límites); guardas;
remedios de error; carga del módulo como lo hace el cargador TOD; y el código real del controlador contra
un sensor simulado (apertura, recuperaciones, captura, cancelación, límites, enroll de 12 toques con
progreso, reintentos, espera de levantar y cancelación a mitad). La normalización da los mismos 7800 bytes
que `normalized()` de numpy sobre una trama sintética. Las tramas sintéticas no contienen datos biométricos.
El comparador se prueba con texturas sintéticas: la misma superficie desplazada se acepta, otras diez se
rechazan, las plantillas dañadas (vacía, ajena, truncada, con bytes de más) dan `DATA_INVALID` con su motivo y
una imagen sin rasgos falla de forma controlada. Las dieciséis pruebas también pasan con
`-Db_sanitize=address,undefined` y detección de fugas.

`build/tests/match-eval` (no se compila por defecto) mide el comparador sobre toques reales guardados en el
equipo; esos toques nunca entran al repositorio.

## Pendiente

- Subir la aceptación del dedo propio (9 de 13 por toque): más toques en el enroll o actualizar la plantilla
  con toques verificados.
- Calibrar en el lector real el umbral de parecido entre toques (`ELAN_ENROLL_SIMILAR_MAX`, hoy 0,9): el log
  `touch for stage N` muestra la cobertura y el parecido de cada toque.
- Licencia: LGPL-2.1-or-later (identificador SPDX en cada fichero; texto completo en `LICENSE`).

## Control de Cambios

| Versión | Fecha | Descripción |
|---|---|---|
| 0.1.0 | 2026-10-12 | Primera versión: protocolo, captura, recuperación, clasificador y pruebas |
| 0.2.0 | 2026-10-13 | Enroll de 12 toques con progreso, reintentos y espera de levantar; print versionado |
| 0.3.0 | 2026-10-13 | Comparador SIFT y RANSAC con OpenCV; plantilla sin imágenes; medido en el lector real; paquete .deb y perfil PAM; README bilingüe (inglés y español) |
| 0.3.1 | 2026-10-13 | El resultado de cada toque queda en el journal (puntaje, nunca la imagen); el paquete fija la serie de libfprint con la que se probó |
