#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Тесты net_speed. Запуск: python -m unittest test_net_speed -v (из папки net_speed)."""

import io
import threading
import time
import unittest
import urllib.error
from contextlib import redirect_stdout
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import net_speed

BLOB_SIZE = 256 * 1024          # 256 КБ — достаточно, чтобы чтение шло несколькими чанками
BLOB = b"x" * BLOB_SIZE
SLOW_DELAY = 1.0                # «зависание» обработчика /slow, секунд


class Handler(BaseHTTPRequestHandler):
    """Маршруты тестового сервера.

    /blob  — 200, тело фиксированного размера BLOB_SIZE
    /fail  — 500
    /flaky — нечётные обращения 500, чётные — как /blob
    /slow  — заголовки отправлены, тело не приходит (для проверки таймаута)
    """

    flaky_calls = 0

    def do_GET(self):
        if self.path == "/blob":
            self._send_blob()
        elif self.path == "/fail":
            self.send_error(500, "Internal Server Error")
        elif self.path == "/flaky":
            Handler.flaky_calls += 1
            if Handler.flaky_calls % 2 == 1:
                self.send_error(500, "Internal Server Error")
            else:
                self._send_blob()
        elif self.path == "/slow":
            self.send_response(200)
            self.send_header("Content-Length", "1024")
            self.end_headers()
            time.sleep(SLOW_DELAY)   # тело так и не отправляем
        else:
            self.send_error(404, "Not Found")

    def _send_blob(self):
        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(BLOB_SIZE))
        self.end_headers()
        self.wfile.write(BLOB)

    def log_message(self, *args):
        """Не засорять вывод тестов логом сервера."""


class ServerTestCase(unittest.TestCase):
    """Базовый класс: поднимает локальный HTTP-сервер один раз на класс."""

    @classmethod
    def setUpClass(cls):
        cls.server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        cls.server.daemon_threads = True
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        host, port = cls.server.server_address
        cls.base_url = f"http://{host}:{port}"

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()

    def setUp(self):
        Handler.flaky_calls = 0


class ParseArgumentsTest(unittest.TestCase):
    """Разбор аргументов вида key=value."""

    def test_only_url_uses_defaults(self):
        url, count, timeout = net_speed.parse_arguments(["url=http://example.com/big.jpg"])
        self.assertEqual(url, "http://example.com/big.jpg")
        self.assertEqual(count, net_speed.DEFAULT_COUNT)
        self.assertEqual(timeout, net_speed.DEFAULT_TIMEOUT)

    def test_custom_count_and_timeout(self):
        url, count, timeout = net_speed.parse_arguments(
            ["url=https://example.com/big.jpg", "count=3", "timeout=5"]
        )
        self.assertEqual(url, "https://example.com/big.jpg")
        self.assertEqual(count, 3)
        self.assertEqual(timeout, 5.0)

    def test_invalid_arguments_raise_value_error(self):
        cases = {
            "нет аргументов": [],
            "нет url": ["count=3"],
            "схема не http(s)": ["url=ftp://example.com/f"],
            "count не число": ["url=http://x/f", "count=abc"],
            "count меньше 1": ["url=http://x/f", "count=0"],
            "timeout не число": ["url=http://x/f", "timeout=abc"],
            "timeout не положительный": ["url=http://x/f", "timeout=0"],
            "неизвестный ключ": ["url=http://x/f", "foo=bar"],
        }
        for name, argv in cases.items():
            with self.subTest(name):
                with self.assertRaises(ValueError):
                    net_speed.parse_arguments(argv)


class CalculateStatsTest(unittest.TestCase):
    """Среднее время, суммарный объём и скорость по успешным запросам."""

    def test_stats_on_known_numbers(self):
        # два запроса: 1 с и 3 с, по 5 000 000 байт каждый
        results = [(1.0, 5_000_000), (3.0, 5_000_000)]
        stats = net_speed.calculate_stats(results)
        self.assertAlmostEqual(stats["avg_time"], 2.0)
        self.assertEqual(stats["total_bytes"], 10_000_000)
        # 10 МБ за 4 с = 2.5 МБ/с = 20 Мбит/с
        self.assertAlmostEqual(stats["mbytes_per_sec"], 2.5)
        self.assertAlmostEqual(stats["mbits_per_sec"], 20.0)

    def test_empty_results_raise(self):
        with self.assertRaises(ValueError):
            net_speed.calculate_stats([])


class DownloadOnceTest(ServerTestCase):
    """Один запрос: время и число реально полученных байт."""

    def test_returns_elapsed_and_received_bytes(self):
        elapsed, received = net_speed.download_once(self.base_url + "/blob", timeout=5)
        self.assertGreater(elapsed, 0.0)
        self.assertEqual(received, BLOB_SIZE)

    def test_http_error_is_raised(self):
        with self.assertRaises(urllib.error.HTTPError):
            net_speed.download_once(self.base_url + "/fail", timeout=5)

    def test_timeout_is_raised(self):
        # URLError и TimeoutError — оба наследники OSError
        with self.assertRaises(OSError):
            net_speed.download_once(self.base_url + "/slow", timeout=0.2)


class DescribeErrorTest(unittest.TestCase):
    """Короткое человекочитаемое описание ошибки запроса."""

    def test_http_error(self):
        exc = urllib.error.HTTPError("http://x", 503, "Service Unavailable", {}, None)
        self.assertEqual(net_speed.describe_error(exc), "HTTP 503 Service Unavailable")

    def test_url_error_uses_reason(self):
        exc = urllib.error.URLError("Name or service not known")
        self.assertEqual(net_speed.describe_error(exc), "Name or service not known")

    def test_plain_os_error(self):
        self.assertEqual(net_speed.describe_error(TimeoutError("timed out")), "timed out")

    def test_error_without_message_falls_back_to_class_name(self):
        self.assertEqual(net_speed.describe_error(ConnectionResetError()), "ConnectionResetError")


class RunMeasurementsTest(ServerTestCase):
    """Серия запросов: сбор успешных, пропуск ошибочных, построчный лог."""

    def _run(self, path, count, timeout=5):
        output = io.StringIO()
        with redirect_stdout(output):
            results = net_speed.run_measurements(self.base_url + path, count, timeout)
        return results, output.getvalue()

    def test_all_successful(self):
        results, output = self._run("/blob", count=3)
        self.assertEqual(len(results), 3)
        for elapsed, received in results:
            self.assertGreater(elapsed, 0.0)
            self.assertEqual(received, BLOB_SIZE)
        self.assertEqual(output.count("| request  |"), 3)
        self.assertNotIn("error", output)

    def test_failed_requests_are_skipped(self):
        results, output = self._run("/flaky", count=4)
        self.assertEqual(len(results), 2)                  # 1-й и 3-й — 500, 2-й и 4-й — ok
        self.assertEqual(output.count("| error"), 2)
        self.assertIn("HTTP 500", output)

    def test_all_failed_returns_empty_list(self):
        results, output = self._run("/fail", count=2)
        self.assertEqual(results, [])
        self.assertEqual(output.count("| error"), 2)


class MainTest(ServerTestCase):
    """Точка входа: код возврата и итоговый отчёт."""

    def _main(self, argv):
        output = io.StringIO()
        with redirect_stdout(output):
            code = net_speed.main(argv)
        return code, output.getvalue()

    def test_success_prints_report_and_returns_zero(self):
        code, output = self._main([f"url={self.base_url}/blob", "count=2"])
        self.assertEqual(code, 0)
        self.assertIn("success   | 2 / 2", output)
        self.assertIn("avg time  |", output)
        self.assertIn(f"total     | {2 * BLOB_SIZE} bytes", output)
        self.assertIn("MB/s", output)
        self.assertIn("Mbit/s", output)

    def test_partial_failures_still_report(self):
        code, output = self._main([f"url={self.base_url}/flaky", "count=4"])
        self.assertEqual(code, 0)
        self.assertIn("success   | 2 / 4", output)

    def test_all_failed_returns_one(self):
        code, output = self._main([f"url={self.base_url}/fail", "count=2"])
        self.assertEqual(code, 1)
        self.assertIn("ни один запрос", output)

    def test_bad_arguments_return_one_with_usage(self):
        code, output = self._main([])
        self.assertEqual(code, 1)
        self.assertIn("Использование:", output)


if __name__ == "__main__":
    unittest.main()
