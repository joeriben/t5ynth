#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

/**
 * 768-channel mixing console for embedding dimension exploration.
 *
 * Shows vertical bars sorted by |A-B difference| (most significant left).
 * Green = A-side, orange = B-side. Draggable bars set per-dimension offsets.
 * Auto-switches: A-B diff when prompt B present, absolute when single prompt.
 */
class DimensionExplorer : public juce::Component
{
public:
    DimensionExplorer();
    ~DimensionExplorer() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Set embedding data from a generation result. */
    void setEmbeddings(const std::vector<float>& embA, const std::vector<float>& embB);

    /** Clear all data and offsets. */
    void clear();

    /** Get current dimension offsets (only non-zero entries). */
    std::vector<std::pair<int, float>> getDimensionOffsets() const;

    /** Returns true when user has made offset edits. */
    bool hasOffsets() const { return hasUserEdits_; }

    /** Undo last edit. */
    void undo();

    /** Redo last undone edit. */
    void redo();

    /** Set overlay mode (interactive bars). In mini-view, clicks just trigger onClicked. */
    void setOverlayMode(bool overlay) { overlayMode_ = overlay; }

    // Callbacks
    std::function<void()> onClicked;          // mini-view clicked → open overlay
    std::function<void()> onApplyGenerate;    // "Anwenden + generieren" pressed

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    static constexpr int kNumDims = 768;

    // Raw embeddings from last generation
    std::vector<float> embA_, embB_;
    bool hasBPrompt_ = false;

    // Sorted bar data
    struct Bar {
        int dimIndex;        // original dimension index (0-767)
        float baseValue;     // original diff or absolute value
        float offset;        // user-applied offset
        float displayValue() const { return baseValue + offset; }
    };
    std::vector<Bar> bars_;

    // Undo/redo
    struct UndoState { std::vector<float> offsets; };
    std::vector<UndoState> undoStack_;
    int undoPos_ = -1;  // current position in undo stack
    void pushUndoState();

    bool overlayMode_ = false;
    bool hasUserEdits_ = false;
    int hoveredBar_ = -1;
    int dragBar_ = -1;
    float dragStartY_ = 0.0f;
    float dragStartValue_ = 0.0f;

    void rebuildBars();
    int barAtX(float x) const;
    float valueToY(float value) const;
    float yToValue(float y) const;
    juce::Rectangle<float> barArea_;  // cached bar drawing area

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DimensionExplorer)
};
