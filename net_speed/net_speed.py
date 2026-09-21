#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Замер скорости скачивания: N последовательных GET-запросов к одному адресу."""

import sys
import time
import http.client
import urllib.error
import urllib.request

DEFAULT_COUNT = 10        # число запросов по умолчанию (по условию задания)
DEFAULT_TIMEOUT = 30.0    # таймаут одного запроса, секунд
CHUNK_SIZE = 64 * 1024    # размер блока чтения тела ответа
BYTES_PER_MB = 1_000_000  # 1 МБ = 10^6 байт (СИ, как у провайдеров)

USAGE = "Использование: python net_speed.py url=<адрес> [count=10] [timeout=30]"


def parse_arguments(argv):
    """Разбор аргументов вида key=value. Возвращает (url, count, timeout).

    Бросает ValueError с понятным сообщением при любой ошибке.
    """
    url = None
    count = DEFAULT_COUNT
    timeout = DEFAULT_TIMEOUT

    for arg in argv:
        if arg.startswith("url="):
            url = arg[len("url="):]
        elif arg.startswith("count="):
            value = arg[len("count="):]
            try:
                count = int(value)
            except ValueError:
                raise ValueError(f"count должен быть целым числом, получено: {value!r}")
        elif arg.startswith("timeout="):
            value = arg[len("timeout="):]
            try:
                timeout = float(value)
            except ValueError:
                raise ValueError(f"timeout должен быть числом секунд, получено: {value!r}")
        else:
            raise ValueError(f"Неизвестный аргумент: {arg!r}")

    if not url:
        raise ValueError("Не указан обязательный параметр url=")
    if not url.startswith(("http://", "https://")):
        raise ValueError("url должен начинаться с http:// или https://")
    if count < 1:
        raise ValueError("count должен быть не меньше 1")
    if timeout <= 0:
        raise ValueError("timeout должен быть положительным")

    return url, count, timeout


def download_once(url, timeout):
    """Один GET-запрос. Возвращает (секунды, полученные байты).

    Время — от начала запроса до дочитывания тела. Считаются реально полученные
    байты, а не Content-Length. Исключения (OSError, http.client.HTTPException)
    пробрасываются вызывающему.
    """
    request = urllib.request.Request(url, headers={
        "Cache-Control": "no-cache",   # не брать ответ из кэша промежуточных прокси
        "User-Agent": "net_speed/1.0",
    })

    received = 0
    start = time.perf_counter()
    with urllib.request.urlopen(request, timeout=timeout) as response:
        while True:
            chunk = response.read(CHUNK_SIZE)
            if not chunk:
                break
            received += len(chunk)
    elapsed = time.perf_counter() - start

    return elapsed, received


def describe_error(exc):
    """Короткое описание ошибки запроса для строки лога."""
    if isinstance(exc, urllib.error.HTTPError):
        return f"HTTP {exc.code} {exc.reason}"
    if isinstance(exc, urllib.error.URLError):
        return str(exc.reason)
    return str(exc) or exc.__class__.__name__


def run_measurements(url, count, timeout):
    """Выполняет count последовательных запросов, печатая строку на каждый.

    Возвращает список (секунды, байты) только для успешных запросов;
    ошибочные запросы печатаются и пропускаются.
    """
    width = len(str(count))  # ширина колонки с номером запроса
    results = []

    for number in range(1, count + 1):
        try:
            elapsed, received = download_once(url, timeout)
        except (OSError, http.client.HTTPException) as exc:
            # OSError покрывает URLError/HTTPError, TimeoutError, ConnectionError;
            # HTTPException — IncompleteRead, RemoteDisconnected и т.п.
            print(f"{number:<{width}} | request  | error    | {describe_error(exc)}")
            continue

        results.append((elapsed, received))
        print(f"{number:<{width}} | request  | {elapsed:6.3f} s | {received} bytes")

    return results


def calculate_stats(results):
    """Статистика по списку успешных запросов [(секунды, байты), ...].

    Скорость считается как суммарный объём / суммарное время — это устойчивее,
    чем среднее из скоростей отдельных запросов.
    """
    if not results:
        raise ValueError("Нет успешных запросов для расчёта статистики")

    total_time = sum(elapsed for elapsed, _ in results)
    total_bytes = sum(received for _, received in results)
    avg_time = total_time / len(results)
    mbytes_per_sec = total_bytes / total_time / BYTES_PER_MB if total_time > 0 else 0.0

    return {
        "avg_time": avg_time,
        "total_bytes": total_bytes,
        "mbytes_per_sec": mbytes_per_sec,
        "mbits_per_sec": mbytes_per_sec * 8,
    }


def print_report(count, results, stats):
    """Итоговая таблица в стиле остальных скриптов проекта."""
    total_mb = stats["total_bytes"] / BYTES_PER_MB
    print("-" * 40)
    print(f"success   | {len(results)} / {count}")
    print(f"avg time  | {stats['avg_time']:.3f} s")
    print(f"total     | {stats['total_bytes']} bytes ({total_mb:.2f} MB)")
    print(f"speed     | {stats['mbytes_per_sec']:.2f} MB/s")
    print(f"speed     | {stats['mbits_per_sec']:.2f} Mbit/s")


def main(argv=None):
    """Точка входа. Возвращает код завершения процесса."""
    if argv is None:
        argv = sys.argv[1:]

    try:
        url, count, timeout = parse_arguments(argv)
    except ValueError as exc:
        print(f"Ошибка: {exc}")
        print(USAGE)
        return 1

    print(f"url       | {url}")
    print(f"count     | {count}")
    print(f"timeout   | {timeout:g} s")
    print("-" * 40)

    try:
        results = run_measurements(url, count, timeout)
    except KeyboardInterrupt:
        print("\nПрервано пользователем")
        return 130

    if not results:
        print("Ошибка: ни один запрос не выполнен успешно")
        return 1

    print_report(count, results, calculate_stats(results))
    return 0


if __name__ == "__main__":
    sys.exit(main())
