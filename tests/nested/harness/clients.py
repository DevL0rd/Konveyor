#!/usr/bin/env python3
import sys

from kwinsession import open_client


def main():
    client, *titles = sys.argv[1:]
    for title in titles:
        open_client(title, client=client)


if __name__ == "__main__":
    main()
