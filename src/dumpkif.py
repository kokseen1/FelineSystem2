from catsys.crypt import (
    CS2_ENCODING,
    beaufort_decipher,
    beaufort_key,
    get_blowfish,
    read_cs2codes,
    vcode_seed,
)
from catsys.mt19937 import mt_genrand
from pathlib import Path
import sys, os
import argparse

NULL = b"\x00"
KIF_SIGNATURE = b"KIF" + NULL
KEY_FILENAME = b"__key__.dat"
ENTRY_SIZE = 64 + 4 + 4
OUT_FNAME = "kif.fs2"


def parse_kif_header(f, raw=False):
    # Verify signature
    sig = f.read(4)
    assert sig == KIF_SIGNATURE

    # Get num entries
    num_entries_raw = f.read(4)
    if raw:
        return num_entries_raw
    return int.from_bytes(num_entries_raw, "little", signed=False)


def get_kif_entry(f):
    data = f.read(ENTRY_SIZE)
    if not data:
        raise Exception("Failed to read KIF entry!")
    return data[:-8], data[-8:]


def gen_file_key(seed):
    return mt_genrand(int.from_bytes(seed, "little")).to_bytes(4, "little")


def dump_kif(kif_path: Path, out, toc_seed):
    bf = None
    num_entries = 0

    with open(kif_path, "rb") as f:
        num_entries = parse_kif_header(f)
        print(f"Parsing {num_entries} entries in {kif_path.name}")

        # Parse entry table
        for i in range(num_entries):
            fname_raw, metadata_raw = get_kif_entry(f)

            # Initialize blowfish
            if fname_raw.rstrip(NULL) == KEY_FILENAME:
                key = gen_file_key(metadata_raw[4:])
                bf = get_blowfish(key)
                continue

            if bf:
                if not toc_seed:
                    print(
                        f"`toc_seed` required for decoding {kif_path.name} not found!"
                    )
                    return 0

                fname = beaufort_decipher(fname_raw, beaufort_key(toc_seed, i))
                metadata = bf.decrypt(
                    (int.from_bytes(metadata_raw, "little") + i).to_bytes(8, "little")
                )
            else:
                # Unencrypted archive
                fname = fname_raw.rstrip(NULL).decode()
                metadata = metadata_raw

            out.write(fname.encode(CS2_ENCODING) + NULL + metadata)

    return num_entries - 1 if bf else num_entries


def dump_kifs(game_dir: Path, db_path: Path):
    # Find game binary
    bin_paths = []
    for ext in ["exe", "bin"]:
        bin_paths.extend(game_dir.glob(f"*.{ext}"))

    toc_seed = None

    # Find VCODE2
    for bin_path in bin_paths:
        vcode2 = read_cs2codes(bin_path).vcode2
        if not vcode2:
            continue

        print(f"Found VCODE2 in {bin_path.name}: {vcode2}")
        toc_seed = vcode_seed(vcode2)

    # Find KIF files
    kif_paths = list(game_dir.glob("*.int"))
    if not kif_paths:
        print(f"No archives in {game_dir.resolve()}")
        return

    total_dumped = 0

    with open(db_path, "wb") as out:
        # Write NULL delimited index table
        for kif_path in kif_paths:
            encrypted = False
            with open(kif_path, "rb") as f:
                num_entries = parse_kif_header(f)
                fname_raw, metadata_raw = get_kif_entry(f)
                # Assumes key is always the first entry
                if fname_raw.rstrip(NULL) == KEY_FILENAME:
                    num_entries -= 1
                    encrypted = True
                out.write(kif_path.name.encode())
                out.write(NULL)
                out.write(int.to_bytes(num_entries, 4, "little", signed=False))
                if encrypted:
                    out.write(b"\x01")
                    out.write(gen_file_key(metadata_raw[4:]))
                else:
                    out.write(b"\x00")

        # Signify end of table
        out.write(NULL)

        # Dump KIF entries
        for kif_idx, kif_path in enumerate(kif_paths):
            total_dumped += dump_kif(kif_path, out, toc_seed)

    print(f"Dumped {total_dumped} entries to {db_path.resolve()}")


def main():
    parser = argparse.ArgumentParser(description="Dump KIF archive entries")

    # Required args
    parser.add_argument(
        "game_dir", help="Path to the game directory containing .int files", type=str
    )

    # Optional args
    parser.add_argument("out_dir", nargs="?", help="Path of output db file", type=str)

    args = parser.parse_args()

    game_dir_path = Path(args.game_dir)

    if not args.out_dir:
        out_db_path = game_dir_path
    else:
        out_db_path = Path(args.out_dir)

    if out_db_path.is_dir():
        out_db_path /= OUT_FNAME

    dump_kifs(game_dir_path, out_db_path)


if __name__ == "__main__":
    main()
