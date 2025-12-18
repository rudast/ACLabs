# Информация по работе

## Участники
- Хаертдинов Руслан Рузилевич - 24КНТ6
- Габидуллин Вильдан Марселевич - 24КНТ7


## Дедлайн
`16.10.2025`

## Запуск основного файла

С параметрами по умолчанию (`X=70`, `N=0`)
```bash
./checker.sh "path_to_log" "path_to_backup"
```

Запуск с указанием `X`
```bash
./checker.sh "path_to_log" "path_to_backup" X
```

Запуск с указанием `N`
```bash
./checker.sh "path_to_log" "path_to_backup" X N
```

#### Пример запуска

```bash
./checker.sh /run/media/rudast/w1/log /run/media/rudast/w2/backup
```

## Запуск тестов
```bash
./testing.sh "path_to_log" "path_to_backup"
```


#### Пример запуска

```bash
./testing.sh /run/media/rudast/w1/log /run/media/rudast/w2/backup
```

После чего в файле `test.log` отобразятся полные логи тестирования, а в консоле будет небольшая сводка по тестам.