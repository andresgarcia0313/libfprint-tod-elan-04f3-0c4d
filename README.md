# tod-elan0c4d

Idioma: es-CO

Módulo externo (TOD) de libfprint para el lector de huellas ELAN 04f3:0c4d, revisión bcdDevice 0x2410.
Es la capa de protocolo portada a C desde la referencia en Python. El comparador de huellas todavía no está
decidido: queda detrás de `src/elan0c4d-match.h`.

## Estado

- Compila con `-Wall -Werror` y exporta `fpi_tod_shared_driver_get_type` (`nm -D` lo muestra).
- Es un `FpDevice` (no `FpImageDevice`): el sensor es de 150x52 y bozorth no sirve.
- `usb_discover` devuelve 100 solo si `g_usb_device_get_release()` es 0x2410; en otra revisión devuelve 0.
- Nunca se probó contra el lector real. Las pruebas usan un sensor simulado (ver abajo).
- El enroll pide 12 toques, como un celular, y reporta cada uno a fprintd (KDE muestra el avance). Un toque
  con cobertura menor de 0,8 se reporta como `FP_DEVICE_RETRY_CENTER_FINGER` y uno igual al anterior (el dedo
  no se movió) como `FP_DEVICE_RETRY_REMOVE_FINGER`; ninguno de los dos cuenta. Entre toques espera a que el
  dedo se levante, sin plazo propio: termina cuando fprintd cancela.
- Al final el enroll entrega los 12 toques al comparador; verify e identify capturan un dedo y también lo
  llaman. El stub responde `FP_DEVICE_ERROR_NOT_SUPPORTED`.

## Compilar y probar

    meson setup build
    ninja -C build
    meson test -C build

No instala nada. Para instalar, copiar `build/libelan0c4d-tod.so` al directorio de módulos TOD
(`FP_TOD_DRIVERS_DIR` o el de la distribución) y reiniciar fprintd.

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

Cada operación es una máquina de estados (`FpiSsm`) sobre `fpi_usb_transfer`. No hay excepciones: cada paso
termina con un `GError` que el nivel superior decide si reintenta (con su remedio) o propaga. Las
cancelaciones y los dispositivos desaparecidos nunca se reintentan. Ante 0xFF en una espera se reenvía el
comando; no se sigue leyendo en EP 0x83. Un tiempo agotado o una cancelación en la espera envían 00 0b.

El `FpPrint` guarda en `fpi-data` un `GVariant` `(qqqaayay)`: versión de formato (1), ancho, alto, los toques
aceptados normalizados a 8 bits y la plantilla del comparador. Un print de otra versión o geometría se
rechaza con `FP_DEVICE_ERROR_DATA_INVALID`.

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
Las catorce pruebas también pasan con `-Db_sanitize=address,undefined`.

## Pendiente

- Probar en el lector real (no se tocó el dispositivo).
- Elegir el comparador e implementar `elan0c4d-match.h`.
- Calibrar en el lector real el umbral de parecido entre toques (`ELAN_ENROLL_SIMILAR_MAX`, hoy 0,9): el log
  `touch for stage N` muestra la cobertura y el parecido de cada toque.
- Licencia: LGPL-2.1-or-later (identificador SPDX en cada fichero; el texto está en
  `/usr/share/common-licenses/LGPL-2.1`).

## Control de Cambios

| Versión | Fecha | Descripción |
|---|---|---|
| 0.1.0 | 2026-10-12 | Primera versión: protocolo, captura, recuperación, clasificador y pruebas |
| 0.2.0 | 2026-10-13 | Enroll de 12 toques con progreso, reintentos y espera de levantar; print versionado |
