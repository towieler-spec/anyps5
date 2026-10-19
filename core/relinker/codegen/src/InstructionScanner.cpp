#include <codegen/IInstructionScanner.hpp>
#include <codegen/CodegenException.hpp>
#include <codegen/x86/X64InstructionDecoder.hpp>
#include <algorithm>
#include <memory>
#include <limits>
#include <string>

namespace Codegen {

class InstructionScanner : public IInstructionScanner {
public:
    explicit InstructionScanner(std::function<void(std::uint64_t, std::uint64_t)> progress, bool resyncUndecodable) : progress(std::move(progress)), resyncUndecodable(resyncUndecodable) {}
    [[nodiscard]] std::vector<InstructionMatch> ScanCodeSection(const std::vector<std::uint8_t>& codeSection, Domain::FileByteOffset codeSectionOffset, Domain::FileByteOffset codeSectionSize) const override;
    void VisitCodeSection(const std::vector<std::uint8_t>& codeSection, Domain::FileByteOffset codeSectionOffset, Domain::FileByteOffset codeSectionSize, const std::function<void(const InstructionMatch&)>& visit) const override;

private:
    std::function<void(std::uint64_t, std::uint64_t)> progress;
    bool resyncUndecodable;
};

template<typename TVisitor>
void ScanCodeSection(const std::vector<std::uint8_t>& codeSection, const Domain::FileByteOffset codeSectionOffset, const Domain::FileByteOffset codeSectionSize, const std::function<void(std::uint64_t, std::uint64_t)>& progress, TVisitor&& visit, const bool resyncUndecodable) {
    if (codeSectionSize > codeSection.size()) throw CodegenException("Code section exceeds the input buffer", codeSectionOffset);
    if (codeSectionSize > std::numeric_limits<Domain::FileByteOffset>::max() - codeSectionOffset) throw CodegenException("Code section offset overflow", codeSectionOffset);
    const auto limit = static_cast<std::size_t>(codeSectionSize);

    const X64InstructionDecoder decoder;
    std::size_t i = 0;

    const auto resync = [&](std::size_t from) {
        for (std::size_t candidate = (from + 16) & ~std::size_t{15}; candidate < limit; candidate += 16) {
            if (candidate >= 2 && codeSection[candidate - 1] == 0xcc && codeSection[candidate - 2] == 0xcc) return candidate;
        }
        return from + 1;
    };

    if (progress) progress(0, limit);
    std::size_t lastReport = 0;
    while (i < limit) {
        const std::uint8_t* cursor = codeSection.data() + i;
        const std::size_t available = limit - i;

        std::size_t length = 0;
        try {
            length = decoder.Decode(cursor, available);
        } catch (const CodegenException& e) {
            if (std::all_of(cursor, cursor + available, [](const std::uint8_t b) { return b == 0; })) break;
            if (!resyncUndecodable) throw CodegenException(std::string("Cannot decode instruction: ") + e.what(), codeSectionOffset + i);
            i = resync(i);
            continue;
        }

        const InstructionMatch match{codeSectionOffset + i, length};
        visit(match);

        i += length;
        if (progress && i - lastReport >= 65536) {
            progress(i, limit);
            lastReport = i;
        }
    }

    if (progress) progress(limit, limit);
}

std::vector<InstructionMatch> InstructionScanner::ScanCodeSection(const std::vector<std::uint8_t>& codeSection, const Domain::FileByteOffset codeSectionOffset, const Domain::FileByteOffset codeSectionSize) const {
    std::vector<InstructionMatch> matches;
    Codegen::ScanCodeSection(codeSection, codeSectionOffset, codeSectionSize, progress, [&](const InstructionMatch& match) { matches.push_back(match); }, resyncUndecodable);
    return matches;
}

void InstructionScanner::VisitCodeSection(const std::vector<std::uint8_t>& codeSection, const Domain::FileByteOffset codeSectionOffset, const Domain::FileByteOffset codeSectionSize, const std::function<void(const InstructionMatch&)>& visit) const {
    Codegen::ScanCodeSection(codeSection, codeSectionOffset, codeSectionSize, progress, visit, resyncUndecodable);
}

std::unique_ptr<IInstructionScanner> MakeInstructionScanner(std::function<void(std::uint64_t, std::uint64_t)> progress, const bool resyncUndecodable) {
    return std::make_unique<InstructionScanner>(std::move(progress), resyncUndecodable);
}

}
