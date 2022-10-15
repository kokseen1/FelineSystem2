from catsys.crypt import (
    CS2_ENCODING,
    beaufort_decipher,
    beaufort_key,
    get_blowfish,
    read_cs2codes,
    vcode_seed,
)
from catsys.mt19937 import mt_genrand
import pathlib
import sys, os

KIF_SIGNATURE = b"KIF\x00"
ENTRY_SIZE = 64 + 4 + 4


def get_kif_entry(f):
    data = f.read(ENTRY_SIZE)
    if not data:
        raise Exception("Failed to read KIF entry!")
    return data[:-8], data[-8:]


def dump_kif(kif_path: pathlib.Path, out, toc_seed, kif_idx):
    bf = None
    num_entries = 0

    with open(kif_path, "rb") as f:
        # Verify signature
        sig = f.read(4)
        assert sig == KIF_SIGNATURE

        # Get num entries
        num_entries = int.from_bytes(f.read(4), "little", signed=False)
        print(f"Dumping {num_entries} entries from {kif_path.name}")

        # Parse entry table
        for i in range(num_entries):
            fname_raw, metadata_raw = get_kif_entry(f)

            # Initialize blowfish
            if fname_raw.rstrip(b"\x00") == b"__key__.dat":
                key = mt_genrand(int.from_bytes(metadata_raw[4:], "little")).to_bytes(
                    4, "little"
                )
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
                fname = fname_raw.rstrip(b"\x00").decode()
                metadata = metadata_raw

            out.write(
                fname.encode(CS2_ENCODING)
                + b"\x00"
                + metadata
                + kif_idx.to_bytes(1, "little"),
            )

    return num_entries


def dump_kifs(path_raw, dbpath):
    game_dir = pathlib.Path(path_raw)

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

    with open(dbpath, "wb") as out:
        # Write NULL delimited index table
        out.write(b"\x00".join([p.name.encode() for p in kif_paths]))
        # Signify end of table
        out.write(b"\x00\x00")

        # Dump KIF entries
        for kif_idx, kif_path in enumerate(kif_paths):
            total_dumped += dump_kif(kif_path, out, toc_seed, kif_idx)

    print(f"Dumped {total_dumped} entries to {dbpath}")


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {os.path.basename(__file__)} GAME_DIR")
        return

    dbpath = sys.argv[2] if len(sys.argv) > 2 else "kif.fs2"
    dump_kifs(sys.argv[1], dbpath)


if __name__ == "__main__":
    main()
