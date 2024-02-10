# shell

### Búsqueda en $PATH
_¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?_

**_RTA:_**
La familia de funciones exec proporciona una interfaz para que el usuario pueda invocar las syscalls como execve(2), execvp(3),
execl(3), execle(3) y demas.
Son distintas funciones que proporcionan diferentes formas de especificar los argumentos que van al nuevo programa.

La syscall se encarga de ejecutar el nuevo programa directamente, las funciones por otro lado son llamadas de biblioteca que envuelven la syscall para hacerla API.



_¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?_

**_RTA:_**
La llamada a exec(3) puede fallar por varias razones, por ejemplo:

- La ruta al archivo binario a ejecutar no existe o no es accesible.
- El archivo binario especificado no tiene permiso de ejecución.
- El sistema operativo ha alcanzado el límite de procesos en ejecución.
- El archivo binario especificado no es un archivo ejecutable válido.

La shell a partir del error, usualmente imprimirá el mensaje de error correspondiene en stderr y no se detendrá, seguirá ejecutándose hasta
terminar el proceso y finalmente devolverá el prompt.

---

### Procesos en segundo plano

_Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano._

**_RTA:_**
La utilización de un proceso en segundo plano se realiza a partir de la utilización del símbolo "&" al final del comando a ejecutar.
Los pasos son los siguientes:

- Paso 1: Se crea el proceso hijo utilizando fork()
- Paso 2: El proceso hijo ejecuta el comando.
- Paso 3: El padre espera oportunamente al hijo con waitpid() y el argumento WNOHANG para que el hijo no quede en un estado zombie.
Este argumento WNOHANG provoca que la función waitpid() no sea bloqueante y provoque que el padre se pueda seguir ejecutando de manera normal.
- Paso 4: Al cerrarse la shell, se debe esperar a todos los procesos antes de terminar la ejecución.

---

### Flujo estándar

_Investigar el significado de 2>&1, explicar cómo funciona su forma general._
_Mostrar qué sucede con la salida de cat out.txt en el ejemplo._
_Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1)._

**_RTA:_**
El simbolo se utiliza para redireccionar la salida de error estandar a la misma ubicacion que la salida estandar.

El "2" refiere al file descriptor para la stderr y "&1" refiere al file descriptor para la stdout. Se utiliza el "&" para evitar que
exista un archivo con el nombre "1" y tome ese archivo para la salida del error en vez de la stdout.

Este comando funciona para que cualquier mensaje de error se imprima en la stdout.

Salida de cat out.txt: 

```bash
$ ls -C /home /noexiste >out.txt 2>&1
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
franvm
```

Como el path '/noexiste' no existe, el error generado que iria a stderror, el 2>&1 lo redirecciona a stdout y todo se guarda en out.txt.

Al invertir el orden, en bash, el error se imprime por pantalla en vez de guardarlo en el archivo. Y el path home se guarda en el archivo.
```bash
$ ls -C /home /noexiste >out.txt 2>&1
ls: cannot access '/noexiste': No such file or directory
$ cat out.txt
/home:
franvm
```

En nuestra shell, la salida sigue siendo la misma que antes.

---

### Tuberías múltiples

_Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe_

_¿Cambia en algo?_

_¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación._

**_RTA:_**
El exit code reportado por la shell cuando se ejecuta un pipe, es el exit code del ultimo comando de la tuberia.
En caso de que falle alguno de los comandos de la tuberia, el exit code indicara que el error es en la ejecucion del comando (exit code !=0).

---

### Variables de entorno temporarias


_¿Por qué es necesario hacerlo luego de la llamada a fork(2)?_

**_RTA:_**
Es necesario definir variables de entorno después de la llamada a fork(2) dado que el cambio en la variable de entorno solo afecte en el momento de ejecución, y por ende, solamente al proceso hijo y no al proceso padre.

_En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3)._

_¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué._

**_RTA:_**
No sera el mismo comportamiento que en el primer caso, cuando se pasa un arreglo de env vars como el tercer argumento de las funciones de exec(3), se esta creando un nuevo conjunto de env vars para el proceso en ejecucion.
Las env vars del proceso anterior no estaran en el nuevo proceso ejecutado.

_Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo._

**_RTA:_**
Una posible implementacion seria crear una funcion auxiliar que tome ese arreglo, y agregue o actualice las env vars usando setenv(3), en vez de crear el conjunto de env vars de nuevo.

---

### Pseudo-variables

_Investigar al menos otras tres variables mágicas estándar, y describir su propósito.
Incluir un ejemplo de su uso en bash (u otra terminal similar)._

**_RTA:_**
Una variable magica es '$$', que representa el pid del shell actual.

Ejemplo: 
```bash 
  $ echo $$
  1149
```

Otra variable magica es '$_', que representa el ultimo argumento del comando anterior.

Ejemplo:
```bash
  $ echo "Hola mundo"
  Hola mundo
  $ echo $_
  mundo
```

Por ultimo, la variable magica '$!' representa el pid del ultimo proceso en segundo plano.

Ejemplo:
```bash
    $ sleep 10 &
    [1] 6672
    $ echo $!
    6672
```
---

### Comandos built-in

_¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)_

**_RTA:_**
El comando "cd" siempre se debe ejecutar en built-in y no en un programa externo, debido a que se debe cambiar el directorio actual donde se encuentra la shell, por lo tanto, el proceso debe ser realizado por el proceso padre.

La implementación de "pwd" como un programa externo es posible porque
simplemente lee y muestra el directorio de trabajo actual del proceso.

Sin embargo, el built-in "pwd" en la shell tiene la ventaja de que puede acceder directamente
a la información del estado del shell, lo que puede hacer que la obtención de la ruta del directorio de trabajo actual sea más rápida y eficiente.
De igual forma los comandos "true" y "false" se implementan como comandos incorporados (built-ins) en la shell,
en lugar de como programas externos. Esto se debe para evitar la sobrecarga del sistema de archivos y la latencia de la llamada al sistema correspondiente.

---

### Historial

_¿Cuál es la función de los parámetros MIN y TIME del modo no canónico? ¿Qué se logra en el ejemplo dado al establecer a MIN en 1 y a TIME en 0?_


**_RTA:_**
El parametro MIN controla el numero minimo de caracteres que deben estar disponibles en el buffer de entrada antes del resultado de lectura.
Cuando MIN=0, la lectura puede ser cualquier cantidad de caracteres.
Si MIN=1, en lectura solo se devuelven caracteres si hay al menos un caracter en el buffer de entrada.

El parametro TIME controla el tiempo de esera antes de que se devuelva el resultado de lectura.
Si TIME=0, en lectura se devuelve cualquier cantidad de caracteres en el buffer de entrada.

En el ejemplo dado, solo se devolvera caracteres cuando haya al menos un caracter en el buffer de entrada. En caso contrario, se esperara hasta que se ingrese al menos un caracter.