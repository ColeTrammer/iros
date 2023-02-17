#include <dius/prelude.h>

#include "initrd.h"

namespace iris::initrd {
struct Args {
    di::PathView path { "."_pv };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("initrd"_sv, "Create iris init ramdisk"_sv)
            .flag<&Args::path>('p', "path"_tsv, "Directory path to create from"_sv);
    }
};

struct FSNode {
    usize size { 0 };
    usize directory_entry_offset { 0 };
    u32 block_offset { 0 };
    Type type { Type::Regular };
    di::Path path;
    di::Vector<FSNode> children;

    void compute_sizes() {
        if (this->type != Type::Directory) {
            return;
        }

        // Compute the sizes of each directory entry.
        for (auto& child : this->children) {
            auto entry_size = sizeof(DirectoryEntry);
            entry_size += child.path.filename()->size();
            entry_size = di::align_up(entry_size, directory_entry_align);

            // If the entry would span into the next block, skip to the end of the block.
            auto current_block_end = di::align_up(this->size, block_size);
            if (this->size + entry_size > current_block_end) {
                this->size = current_block_end;
            }

            // Assign the child's directory entry offset based on the current size.
            child.directory_entry_offset = this->size;
            this->size += entry_size;
        }

        // Compute the sizes of any child directory.
        for (auto& child : this->children) {
            child.compute_sizes();
        }
    }

    usize compute_block_offsets(usize starting_offset = 1) {
        this->block_offset = starting_offset;
        starting_offset += di::divide_round_up(this->size, block_size);
        for (auto& child : this->children) {
            starting_offset = child.compute_block_offsets(starting_offset);
        }
        return starting_offset;
    }

    di::Result<void> write_to_disk(dius::SyncFile& output) {
        if (this->type == Type::Regular) {
            auto buffer = di::Array<di::Byte, block_size> {};
            auto file = TRY(dius::open_sync(this->path, dius::OpenMode::Readonly));
            u64 offset = 0;
            while (offset < this->size) {
                auto nread = TRY(file.read_some(*buffer.span().first(di::min(buffer.size(), this->size - offset))));
                if (nread == 0) {
                    break;
                }

                TRY(output.write_exactly(this->block_offset * block_size + offset, { buffer.data(), nread }));
                offset += nread;
            }

            return {};
        }

        // Write out each directory entry in sequence.
        for (auto [i, child] : di::enumerate(this->children)) {
            auto entry = DirectoryEntry {};
            entry.block_offset = child.block_offset;
            entry.byte_size = child.size;
            if (i != this->children.size() - 1) {
                entry.next_entry = this->children[i + 1].directory_entry_offset - child.directory_entry_offset;
            } else {
                entry.next_entry = 0;
            }
            entry.type = type;
            entry.name_length = child.path.filename()->size();

            TRY(output.write_exactly(this->block_offset * block_size + child.directory_entry_offset,
                                     di::as_bytes(di::Span { &entry, 1 })));
            TRY(output.write_exactly(this->block_offset * block_size + child.directory_entry_offset + sizeof(entry),
                                     di::as_bytes(child.path.filename()->span())));
        }

        for (auto& child : this->children) {
            TRY(child.write_to_disk(output));
        }
        return {};
    }
};

static di::Result<void> write_super_block(FSNode& root, dius::SyncFile& output, usize total_blocks) {
    auto super_block = SuperBlock {};
    // FIXME: Chose a proper UUID signature.
    super_block.signature.fill(di::Byte(0x1E));
    // FIXME: Generate a random UUID.
    super_block.generation.fill(di::Byte(0x24));
    // FIXME: Get the current time.
    super_block.created_at_seconds_since_epoch = 0;
    super_block.version = 0;
    super_block.total_size = total_blocks * block_size;
    super_block.root_directory.block_offset = root.block_offset;
    super_block.root_directory.byte_size = root.size;
    super_block.root_directory.next_entry = 0;
    super_block.root_directory.type = Type::Directory;
    super_block.root_directory.name_length = 0;

    return output.write_exactly(0, di::as_bytes(di::Span { &super_block, 1 }));
}

di::Result<FSNode&> find_parent(FSNode& root, di::PathView path) {
    if (path.empty()) {
        return root;
    }

    auto first = *path.begin();
    for (auto& child : root.children) {
        if (child.type == Type::Directory && child.path.filename() == first) {
            return find_parent(child, *path.strip_prefix(first));
        }
    }

    dius::eprintln("Could not find path {} in directory tree."_sv, path);
    return di::Unexpected(di::BasicError::Invalid);
}

di::Result<void> main(Args& args) {
    auto root = FSNode {};
    root.type = Type::Directory;

    auto iterator = TRY(di::create<dius::fs::RecursiveDirectoryIterator>(args.path.to_owned()));
    for (auto directory : iterator) {
        auto const& entry = TRY(directory);

        auto is_invalid = TRY(entry.is_symlink()) || TRY(entry.is_other());
        if (is_invalid) {
            dius::eprintln("Cannot create initrd consisting of irregular file: {}"_sv, entry);
            return di::Unexpected(di::BasicError::Invalid);
        }

        auto is_directory = TRY(entry.is_directory());
        auto child = FSNode {};
        child.path = entry.path_view().to_owned();
        if (!is_directory) {
            child.size = TRY(entry.file_size());
            child.type = Type::Regular;
        } else {
            child.type = Type::Directory;
        }

        dius::println("Adding {}."_sv, entry);

        auto relative_path = entry.path_view().strip_prefix(args.path).value_or(entry.path_view());
        auto parent_path = relative_path.parent_path().value_or(""_pv);

        auto& parent = TRY(find_parent(root, parent_path));
        parent.children.push_back(di::move(child));
    }

    root.compute_sizes();
    auto total_blocks = root.compute_block_offsets();

    dius::println("root directory entry size: {}"_sv, root.size);
    dius::println("total blocks: {}"_sv, total_blocks);

    auto output = TRY(dius::open_sync("initrd.bin"_tsv, dius::OpenMode::WriteClobber));
    TRY(output.resize_file(total_blocks * block_size));
    TRY(write_super_block(root, output, total_blocks));
    return root.write_to_disk(output);
}
}

DIUS_MAIN(iris::initrd::Args, iris::initrd)
