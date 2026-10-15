#include "Ngs2Test.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>

static uintptr_t Sampler(uintptr_t system, std::uint32_t rate = 48000) {
    const auto voice = Voice(CreateRack(system, SCE_NGS2_RACK_ID_SAMPLER));
    Control(voice, SCE_NGS2_SAMPLER_VOICE_PARAM_SETUP,
            Ngs2SamplerVoiceSetupParam{{}, {SCE_NGS2_WAVEFORM_TYPE_PCM_I16L, 1, rate, 0, 0, 0}});
    Patch(voice, Mastering(system, 1));
    return voice;
}

static void Queue(uintptr_t voice, const std::int16_t* data, std::uint32_t frames,
                  std::uint32_t flags = SCE_NGS2_WAVEFORM_BLOCKS_FLAG_CONTINUE,
                  std::uint32_t declared = 0) {
    const Ngs2WaveformBlock block{0, frames * sizeof(std::int16_t), 0, 0, declared ? declared : frames, 0, 0};
    Control(voice, SCE_NGS2_SAMPLER_VOICE_PARAM_ADD_WAVEFORM_BLOCKS,
            Ngs2SamplerVoiceWaveformBlocksParam{{}, data, flags, 1, &block});
}

static std::array<float, Grain> Render(uintptr_t system) {
    std::array<float, Grain> output{};
    const Ngs2RenderBufferInfo info{output.data(), sizeof(output), SCE_NGS2_WAVEFORM_TYPE_PCM_F32L, 1};
    Require(sceNgs2SystemRender(system, &info, 1) == SCE_NGS2_OK);
    return output;
}

static Ngs2SamplerVoiceState State(uintptr_t voice) {
    Ngs2SamplerVoiceState state{};
    Require(sceNgs2VoiceGetState(voice, &state.voice_state, sizeof(state)) == SCE_NGS2_OK);
    return state;
}

static void Constant(const std::array<float, Grain>& output, std::int16_t value) {
    for (const auto sample : output) Require(std::abs(sample - value / 32768.0f) < 1e-5f);
}

static void TestLateFillAndReleasedInput() {
    const auto system = CreateSystem();
    const auto voice = Sampler(system);
    std::array<std::int16_t, Grain * 2> first, future;
    first.fill(1000); future.fill(2000);
    Queue(voice, first.data(), first.size());
    Queue(voice, future.data(), future.size());
    Require(State(voice).decoded_data_size == 0);
    Event(voice, SCE_NGS2_VOICE_EVENT_PLAY);
    Constant(Render(system), 1000);
    const auto progress = State(voice);
    Require(progress.num_decoded_samples == Grain && progress.decoded_data_size == sizeof(first));
    Require(progress.waveform_data == first.data() + Grain);
    first.fill(-1000);
    future.fill(3000);
    Constant(Render(system), 1000);
    Require(State(voice).decoded_data_size == sizeof(first));
    future.fill(4000);
    Constant(Render(system), 4000);
    Require(State(voice).decoded_data_size == sizeof(first) + sizeof(future));
    future.fill(-4000);
    Constant(Render(system), 4000);
    Require(State(voice).waveform_data == future.data() + future.size());
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
}

static void TestReleasedRegionRing(std::uint32_t rate) {
    const auto system = CreateSystem();
    const auto voice = Sampler(system, rate);
    std::array<std::array<std::int16_t, Grain * 2>, 4> ring{};
    struct Region { std::int16_t* data; std::uint32_t frames; };
    std::deque<Region> pending;
    std::uint32_t generated = 0;
    const auto sample = [](std::uint32_t frame) { return static_cast<std::int16_t>(1000 + (frame % 512) * 32); };
    const auto queue = [&](std::int16_t* data, std::uint32_t frames) {
        for (std::uint32_t i = 0; i < frames; ++i) data[i] = sample(generated++);
        Queue(voice, data, frames);
        pending.push_back({data, frames});
    };
    for (auto& buffer : ring) queue(buffer.data(), buffer.size());
    Event(voice, SCE_NGS2_VOICE_EVENT_PLAY);
    double position = 0;
    std::uint64_t decoded = 0;
    for (int render = 0; render < 100; ++render) {
        for (const auto value : Render(system)) {
            const auto frame = static_cast<std::uint32_t>(position);
            const auto expected = std::lerp(static_cast<float>(sample(frame)), static_cast<float>(sample(frame + 1)), static_cast<float>(position - frame)) / 32768.0f;
            Require(std::abs(value - expected) < 1e-4f);
            position += rate / 48000.0;
        }
        const auto state = State(voice);
        Require(state.decoded_data_size >= state.num_decoded_samples * sizeof(std::int16_t));
        Require(state.decoded_data_size <= generated * sizeof(std::int16_t));
        auto remaining = (state.decoded_data_size - decoded) / sizeof(std::int16_t);
        while (remaining != 0) {
            Require(!pending.empty());
            const auto released = pending.front();
            const auto count = static_cast<std::uint32_t>(std::min<std::uint64_t>(remaining, released.frames));
            pending.front().data += count;
            pending.front().frames -= count;
            if (pending.front().frames == 0) pending.pop_front();
            queue(released.data, count);
            remaining -= count;
        }
        decoded = state.decoded_data_size;
    }
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
}

static void TestStreamPieceLifetime() {
    const auto system = CreateSystem();
    const auto voice = Sampler(system);
    std::array<std::int16_t, 6> first;
    std::array<std::int16_t, Grain * 2> next;
    first.fill(1000); next.fill(2000);
    Queue(voice, first.data(), first.size(), SCE_NGS2_WAVEFORM_BLOCKS_FLAG_CONTINUE, first.size() + next.size());
    Event(voice, SCE_NGS2_VOICE_EVENT_PLAY);
    const auto beginning = Render(system);
    for (std::uint32_t i = 0; i < Grain; ++i) Require(beginning[i] == (i < first.size() ? 1000 / 32768.0f : 0));
    Queue(voice, next.data(), next.size(), SCE_NGS2_WAVEFORM_BLOCKS_FLAG_APPEND);
    next.fill(3000);
    Constant(Render(system), 3000);
    Require(State(voice).decoded_data_size == sizeof(first) + sizeof(next));
    next.fill(-3000);
    Constant(Render(system), 3000);
    Require(State(voice).num_decoded_samples == first.size() + next.size());
    Require(State(voice).waveform_data == next.data() + next.size());
    Require(Flags(voice) == 0);
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
}

int main() {
    TestLateFillAndReleasedInput();
    TestReleasedRegionRing(48000);
    TestReleasedRegionRing(44100);
    TestStreamPieceLifetime();
}
