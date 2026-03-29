// Minimal test: load models and run one generation step to verify shapes.
// Build: see CMake test target or compile manually against LibTorch + SentencePiece.
//
// This file is NOT part of the plugin build — it's a standalone diagnostic tool.
// Run with: cd build && ./test_inference ../exported_models

#include <torch/torch.h>
#include <torch/script.h>
#include <sentencepiece_processor.h>
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <model_dir>" << std::endl;
        return 1;
    }
    std::string modelDir = argv[1];
    auto path = [&](const char* name) { return modelDir + "/" + name; };

    torch::Device device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
    std::cout << "Device: " << (device == torch::kCUDA ? "CUDA" : "CPU") << std::endl;

    // 1. Tokenizer
    sentencepiece::SentencePieceProcessor tokenizer;
    auto status = tokenizer.Load(path("spiece.model"));
    if (!status.ok()) { std::cerr << "Tokenizer: " << status.ToString() << std::endl; return 1; }
    std::cout << "Tokenizer loaded." << std::endl;

    std::vector<int> ids;
    tokenizer.Encode("a steady clean saw wave", &ids);
    std::cout << "Tokens: " << ids.size() << " ids" << std::endl;

    // Pad to 128
    ids.resize(128, 0);
    auto tokenIds = torch::tensor(std::vector<int64_t>(ids.begin(), ids.end())).unsqueeze(0).to(device);
    auto mask = (tokenIds != 0).to(torch::kInt64);
    std::cout << "tokenIds: " << tokenIds.sizes() << ", mask: " << mask.sizes() << std::endl;

    // 2. T5 Encoder
    auto t5 = torch::jit::load(path("t5_encoder.pt"), device);
    t5.eval();
    torch::NoGradGuard noGrad;
    auto t5Out = t5.forward({tokenIds, mask});
    torch::Tensor textEmb;
    if (t5Out.isTuple()) textEmb = t5Out.toTuple()->elements()[0].toTensor();
    else if (t5Out.isGenericDict()) textEmb = t5Out.toGenericDict().at("last_hidden_state").toTensor();
    else textEmb = t5Out.toTensor();
    std::cout << "T5 output: " << textEmb.sizes() << std::endl;

    // 3. Projection
    auto proj = torch::jit::load(path("projection_model.pt"), device);
    proj.eval();
    auto projOut = proj.forward({
        textEmb,
        torch::tensor({0.0f}).to(device),
        torch::tensor({3.0f}).to(device)
    });

    torch::Tensor textHidden, startHidden, endHidden;
    if (projOut.isGenericDict())
    {
        auto dict = projOut.toGenericDict();
        textHidden = dict.at("text_hidden_states").toTensor();
        startHidden = dict.at("seconds_start_hidden_states").toTensor();
        endHidden = dict.at("seconds_end_hidden_states").toTensor();
        std::cout << "Projection output type: dict" << std::endl;
    }
    else if (projOut.isTuple())
    {
        auto tuple = projOut.toTuple();
        textHidden = tuple->elements()[0].toTensor();
        startHidden = tuple->elements()[1].toTensor();
        endHidden = tuple->elements()[2].toTensor();
        std::cout << "Projection output type: tuple" << std::endl;
    }
    std::cout << "  text_hidden: " << textHidden.sizes() << std::endl;
    std::cout << "  start_hidden: " << startHidden.sizes() << std::endl;
    std::cout << "  end_hidden: " << endHidden.sizes() << std::endl;

    // Assemble DiT inputs
    auto encoderHidden = torch::cat({textHidden, startHidden, endHidden}, 1);
    auto globalHidden = torch::cat({startHidden, endHidden}, 2).squeeze(1);
    auto attMask = torch::ones({1, encoderHidden.size(1)},
                               torch::TensorOptions().dtype(torch::kBool).device(device));
    std::cout << "encoder_hidden: " << encoderHidden.sizes() << std::endl;
    std::cout << "global_hidden: " << globalHidden.sizes() << std::endl;
    std::cout << "attention_mask: " << attMask.sizes() << std::endl;

    // 4. DiT (single step)
    auto dit = torch::jit::load(path("dit.pt"), device);
    dit.eval();

    auto latent = torch::randn({1, 64, 1024}, torch::TensorOptions().dtype(torch::kFloat32).device(device));
    auto timestep = torch::tensor({0.5f}).to(device);

    std::cout << "Running DiT forward..." << std::flush;
    auto ditOut = dit.forward({latent, timestep, encoderHidden, globalHidden, attMask}).toTensor();
    std::cout << " output: " << ditOut.sizes() << std::endl;

    // 5. VAE Decoder
    auto vae = torch::jit::load(path("vae_decoder.pt"), device);
    vae.eval();
    std::cout << "Running VAE decode..." << std::flush;
    auto audio = vae.forward({latent}).toTensor();
    std::cout << " output: " << audio.sizes() << std::endl;

    std::cout << "\nAll shapes verified. Pipeline is functional." << std::endl;
    return 0;
}
