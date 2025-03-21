﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ResourceAccessor.h"
#include "RuntimeProfiler.h"
#include "InteractionTrackerOwner.h"
#include "ScrollPresenter.h"
#include "ScrollingScrollOptions.h"
#include "ScrollingZoomOptions.h"
#include "ScrollPresenterAutomationPeer.h"
#include "ScrollPresenterTestHooks.h"
#include "Vector.h"
#include "RegUtil.h"
#include "MuxcTraceLogging.h"

// Change to 'true' to turn on debugging outputs in Output window
bool ScrollPresenterTrace::s_IsDebugOutputEnabled{ false };
bool ScrollPresenterTrace::s_IsVerboseDebugOutputEnabled{ false };

// Number of pixels scrolled when the automation peer requests a line-type change.
const double c_scrollPresenterLineDelta = 16.0;

// Default inertia decay rate used when a IScrollController makes a request for
// an offset change with additional velocity.
const float c_scrollPresenterDefaultInertiaDecayRate = 0.95f;

const int32_t ScrollPresenter::s_noOpCorrelationId{ -1 };

ScrollPresenter::~ScrollPresenter()
{
    SCROLLPRESENTER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookCompositionTargetRendering();
    UnhookScrollPresenterEvents();
}

ScrollPresenter::ScrollPresenter()
{
    SCROLLPRESENTER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ScrollPresenter);

    EnsureProperties();

    winrt::UIElement::RegisterAsScrollPort(*this);

    HookScrollPresenterEvents();

    // Set the default Transparent background so that hit-testing allows to start a touch manipulation
    // outside the boundaries of the Content, when it's smaller than the ScrollPresenter.
    Background(winrt::SolidColorBrush(winrt::Colors::Transparent()));
}

#pragma region Automation Peer Helpers

// Public methods accessed by the ScrollPresenterAutomationPeer class

double ScrollPresenter::GetZoomedExtentWidth() const
{
    return m_unzoomedExtentWidth * m_zoomFactor;
}

double ScrollPresenter::GetZoomedExtentHeight() const
{
    return m_unzoomedExtentHeight * m_zoomFactor;
}

void ScrollPresenter::PageLeft()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToHorizontalOffset(m_zoomedHorizontalOffset - ViewportWidth());
}

void ScrollPresenter::PageRight()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToHorizontalOffset(m_zoomedHorizontalOffset + ViewportWidth());
}

void ScrollPresenter::PageUp()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToVerticalOffset(m_zoomedVerticalOffset - ViewportHeight());
}

void ScrollPresenter::PageDown()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToVerticalOffset(m_zoomedVerticalOffset + ViewportHeight());
}

void ScrollPresenter::LineLeft()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToHorizontalOffset(m_zoomedHorizontalOffset - c_scrollPresenterLineDelta);
}

void ScrollPresenter::LineRight()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToHorizontalOffset(m_zoomedHorizontalOffset + c_scrollPresenterLineDelta);
}

void ScrollPresenter::LineUp()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToVerticalOffset(m_zoomedVerticalOffset - c_scrollPresenterLineDelta);
}

void ScrollPresenter::LineDown()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ScrollToVerticalOffset(m_zoomedVerticalOffset + c_scrollPresenterLineDelta);
}

void ScrollPresenter::ScrollToHorizontalOffset(double offset)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

    ScrollToOffsets(offset /*zoomedHorizontalOffset*/, m_zoomedVerticalOffset /*zoomedVerticalOffset*/);
}

void ScrollPresenter::ScrollToVerticalOffset(double offset)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

    ScrollToOffsets(m_zoomedHorizontalOffset /*zoomedHorizontalOffset*/, offset /*zoomedVerticalOffset*/);
}

void ScrollPresenter::ScrollToOffsets(
    double horizontalOffset, double verticalOffset)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffset, verticalOffset);

    if (m_interactionTracker)
    {
        com_ptr<ScrollingScrollOptions> options =
            winrt::make_self<ScrollingScrollOptions>(
                winrt::ScrollingAnimationMode::Disabled,
                winrt::ScrollingSnapPointsMode::Ignore);

        std::shared_ptr<OffsetsChange> offsetsChange =
            std::make_shared<OffsetsChange>(
                horizontalOffset,
                verticalOffset,
                ScrollPresenterViewKind::Absolute,
                winrt::IInspectable{ *options }); // NOTE: Using explicit cast to winrt::IInspectable to work around 17532876

        ProcessOffsetsChange(
            InteractionTrackerAsyncOperationTrigger::DirectViewChange,
            offsetsChange,
            s_noOpCorrelationId /*offsetsChangeCorrelationId*/,
            false /*isForAsyncOperation*/);
    }
}

#pragma endregion

#pragma region IUIElementOverridesHelper

winrt::AutomationPeer ScrollPresenter::OnCreateAutomationPeer()
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    return winrt::make<ScrollPresenterAutomationPeer>(*this);
}

#pragma endregion

#pragma region IScrollPresenter

winrt::CompositionPropertySet ScrollPresenter::ExpressionAnimationSources()
{
    SetupInteractionTrackerBoundaries();
    EnsureExpressionAnimationSources();

    return m_expressionAnimationSources;
}

double ScrollPresenter::HorizontalOffset() const
{
    return m_zoomedHorizontalOffset;
}

double ScrollPresenter::VerticalOffset() const
{
    return m_zoomedVerticalOffset;
}

float ScrollPresenter::ZoomFactor() const
{
    return m_zoomFactor;
}

double ScrollPresenter::ExtentWidth() const
{
    return m_unzoomedExtentWidth;
}

double ScrollPresenter::ExtentHeight() const
{
    return m_unzoomedExtentHeight;
}

double ScrollPresenter::ViewportWidth() const
{
    return m_viewportWidth;
}

double ScrollPresenter::ViewportHeight() const
{
    return m_viewportHeight;
}

double ScrollPresenter::ScrollableWidth() const
{
    return std::max(0.0, GetZoomedExtentWidth() - ViewportWidth());
}

double ScrollPresenter::ScrollableHeight() const
{
    return std::max(0.0, GetZoomedExtentHeight() - ViewportHeight());
}

// AnticipatedZoomedHorizontalOffset, AnticipatedZoomedVerticalOffset() and AnticipatedZoomFactor() are
// used to evaluate the view for the ScrollStarting/ZoomStarting events raised when a ScrollTo, ScrollBy,
// ZoomTo or ZoomBy request is handed off to the InteractionTracker.

double ScrollPresenter::AnticipatedZoomedHorizontalOffset() const
{
    return isnan(m_anticipatedZoomedHorizontalOffset) ? m_zoomedHorizontalOffset : m_anticipatedZoomedHorizontalOffset;
}

double ScrollPresenter::AnticipatedZoomedVerticalOffset() const
{
    return isnan(m_anticipatedZoomedVerticalOffset) ? m_zoomedVerticalOffset : m_anticipatedZoomedVerticalOffset;
}

float ScrollPresenter::AnticipatedZoomFactor() const
{
    return isnan(m_anticipatedZoomFactor) ? m_zoomFactor : m_anticipatedZoomFactor;
}

double ScrollPresenter::AnticipatedScrollableWidth() const
{
    return std::max(0.0, m_unzoomedExtentWidth * AnticipatedZoomFactor() - ViewportWidth());
}

double ScrollPresenter::AnticipatedScrollableHeight() const
{
    return std::max(0.0, m_unzoomedExtentHeight * AnticipatedZoomFactor() - ViewportHeight());
}

winrt::IScrollController ScrollPresenter::HorizontalScrollController() const
{
    if (m_horizontalScrollController)
    {
        return m_horizontalScrollController.get();
    }

    return nullptr;
}

void ScrollPresenter::HorizontalScrollController(winrt::IScrollController const& value)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_PTR, METH_NAME, this, value);

    if (m_horizontalScrollController)
    {
        UnhookHorizontalScrollControllerEvents();

        if (m_horizontalScrollControllerPanningInfo)
        {
            UnhookHorizontalScrollControllerPanningInfoEvents();

            if (m_horizontalScrollControllerExpressionAnimationSources)
            {
                m_horizontalScrollControllerPanningInfo.get().SetPanningElementExpressionAnimationSources(
                    nullptr /*scrollControllerExpressionAnimationSources*/,
                    s_minOffsetPropertyName,
                    s_maxOffsetPropertyName,
                    s_offsetPropertyName,
                    s_multiplierPropertyName);
            }
        }
    }

    m_horizontalScrollController.set(value);
    m_horizontalScrollControllerPanningInfo.set(value ? value.PanningInfo() : nullptr);

    if (m_interactionTracker)
    {
        SetupScrollControllerVisualInterationSource(ScrollPresenterDimension::HorizontalScroll);
    }

    if (m_horizontalScrollController)
    {
        HookHorizontalScrollControllerEvents(
            m_horizontalScrollController.get());

        if (m_horizontalScrollControllerPanningInfo)
        {
            HookHorizontalScrollControllerPanningInfoEvents(
                m_horizontalScrollControllerPanningInfo.get(),
                m_horizontalScrollControllerVisualInteractionSource != nullptr /*hasInteractionSource*/);
        }

        UpdateScrollControllerValues(ScrollPresenterDimension::HorizontalScroll);
        UpdateScrollControllerIsScrollable(ScrollPresenterDimension::HorizontalScroll);

        if (m_horizontalScrollControllerPanningInfo && m_horizontalScrollControllerExpressionAnimationSources)
        {
            m_horizontalScrollControllerPanningInfo.get().SetPanningElementExpressionAnimationSources(
                m_horizontalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }
}

winrt::IScrollController ScrollPresenter::VerticalScrollController() const
{
    if (m_verticalScrollController)
    {
        return m_verticalScrollController.get();
    }

    return nullptr;
}

void ScrollPresenter::VerticalScrollController(winrt::IScrollController const& value)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_PTR, METH_NAME, this, value);

    if (m_verticalScrollController)
    {
        UnhookVerticalScrollControllerEvents();
    }

    if (m_verticalScrollControllerPanningInfo)
    {
        UnhookVerticalScrollControllerPanningInfoEvents();

        if (m_verticalScrollControllerExpressionAnimationSources)
        {
            m_verticalScrollControllerPanningInfo.get().SetPanningElementExpressionAnimationSources(
                nullptr /*scrollControllerExpressionAnimationSources*/,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }

    m_verticalScrollController.set(value);
    m_verticalScrollControllerPanningInfo.set(value ? value.PanningInfo() : nullptr);

    if (m_interactionTracker)
    {
        SetupScrollControllerVisualInterationSource(ScrollPresenterDimension::VerticalScroll);
    }

    if (m_verticalScrollController)
    {
        HookVerticalScrollControllerEvents(
            m_verticalScrollController.get());

        if (m_verticalScrollControllerPanningInfo)
        {
            HookVerticalScrollControllerPanningInfoEvents(
                m_verticalScrollControllerPanningInfo.get(),
                m_verticalScrollControllerVisualInteractionSource != nullptr /*hasInteractionSource*/);
        }

        UpdateScrollControllerValues(ScrollPresenterDimension::VerticalScroll);
        UpdateScrollControllerIsScrollable(ScrollPresenterDimension::VerticalScroll);

        if (m_verticalScrollControllerPanningInfo && m_verticalScrollControllerExpressionAnimationSources)
        {
            m_verticalScrollControllerPanningInfo.get().SetPanningElementExpressionAnimationSources(
                m_verticalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }
}

winrt::ScrollingInputKinds ScrollPresenter::IgnoredInputKinds() const
{
    // Workaround for Bug 17377013: XamlCompiler codegen for Enum CreateFromString always returns boxed int which is wrong for [flags] enums (should be uint)
    // Check if the boxed IgnoredInputKinds is an IReference<int> first in which case we unbox as int.
    auto boxedKind = GetValue(s_IgnoredInputKindsProperty);
    if (auto boxedInt = boxedKind.try_as<winrt::IReference<int32_t>>())
    {
        return winrt::ScrollingInputKinds{ static_cast<uint32_t>(unbox_value<int32_t>(boxedInt)) };
    }

    return auto_unbox(boxedKind);
}

void ScrollPresenter::IgnoredInputKinds(winrt::ScrollingInputKinds const& value)
{
    SetValue(s_IgnoredInputKindsProperty, box_value(value));
}

winrt::ScrollingInteractionState ScrollPresenter::State() const
{
    return m_state;
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollPresenter::HorizontalSnapPoints()
{
    if (!m_horizontalSnapPoints)
    {
        m_horizontalSnapPoints = winrt::make<Vector<winrt::ScrollSnapPointBase>>();

        auto horizontalSnapPointsObservableVector = m_horizontalSnapPoints.try_as<winrt::IObservableVector<winrt::ScrollSnapPointBase>>();
        m_horizontalSnapPointsVectorChangedRevoker = horizontalSnapPointsObservableVector.VectorChanged(
            winrt::auto_revoke,
            { this, &ScrollPresenter::OnHorizontalSnapPointsVectorChanged });
    }
    return m_horizontalSnapPoints;
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollPresenter::VerticalSnapPoints()
{
    if (!m_verticalSnapPoints)
    {
        m_verticalSnapPoints = winrt::make<Vector<winrt::ScrollSnapPointBase>>();

        auto verticalSnapPointsObservableVector = m_verticalSnapPoints.try_as<winrt::IObservableVector<winrt::ScrollSnapPointBase>>();
        m_verticalSnapPointsVectorChangedRevoker = verticalSnapPointsObservableVector.VectorChanged(
            winrt::auto_revoke,
            { this, &ScrollPresenter::OnVerticalSnapPointsVectorChanged });
    }
    return m_verticalSnapPoints;
}

winrt::IVector<winrt::ZoomSnapPointBase> ScrollPresenter::ZoomSnapPoints()
{
    if (!m_zoomSnapPoints)
    {
        m_zoomSnapPoints = winrt::make<Vector<winrt::ZoomSnapPointBase>>();

        auto zoomSnapPointsObservableVector = m_zoomSnapPoints.try_as<winrt::IObservableVector<winrt::ZoomSnapPointBase>>();
        m_zoomSnapPointsVectorChangedRevoker = zoomSnapPointsObservableVector.VectorChanged(
            winrt::auto_revoke,
            { this, &ScrollPresenter::OnZoomSnapPointsVectorChanged });
    }
    return m_zoomSnapPoints;
}

int32_t ScrollPresenter::ScrollTo(double horizontalOffset, double verticalOffset)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffset, verticalOffset);

    return ScrollTo(horizontalOffset, verticalOffset, nullptr /*options*/);
}

int32_t ScrollPresenter::ScrollTo(double horizontalOffset, double verticalOffset, winrt::ScrollingScrollOptions const& options)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        horizontalOffset, verticalOffset, TypeLogging::ScrollOptionsToString(options).c_str());

    int32_t viewChangeCorrelationId;
    ChangeOffsetsPrivate(
        horizontalOffset /*zoomedHorizontalOffset*/,
        verticalOffset /*zoomedVerticalOffset*/,
        ScrollPresenterViewKind::Absolute,
        options,
        nullptr /*bringIntoViewRequestedEventArgs*/,
        InteractionTrackerAsyncOperationTrigger::DirectViewChange,
        s_noOpCorrelationId /*existingViewChangeCorrelationId*/,
        &viewChangeCorrelationId);

    return viewChangeCorrelationId;
}

int32_t ScrollPresenter::ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffsetDelta, verticalOffsetDelta);

    return ScrollBy(horizontalOffsetDelta, verticalOffsetDelta, nullptr /*options*/);
}

int32_t ScrollPresenter::ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, winrt::ScrollingScrollOptions const& options)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        horizontalOffsetDelta, verticalOffsetDelta, TypeLogging::ScrollOptionsToString(options).c_str());

    int32_t viewChangeCorrelationId;
    ChangeOffsetsPrivate(
        horizontalOffsetDelta /*zoomedHorizontalOffset*/,
        verticalOffsetDelta /*zoomedVerticalOffset*/,
        ScrollPresenterViewKind::RelativeToCurrentView,
        options,
        nullptr /*bringIntoViewRequestedEventArgs*/,
        InteractionTrackerAsyncOperationTrigger::DirectViewChange,
        s_noOpCorrelationId /*existingViewChangeCorrelationId*/,
        &viewChangeCorrelationId);

    return viewChangeCorrelationId;
}

int32_t ScrollPresenter::AddScrollVelocity(winrt::float2 offsetsVelocity, winrt::IReference<winrt::float2> inertiaDecayRate)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::Float2ToString(offsetsVelocity).c_str(), TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());

    int32_t viewChangeCorrelationId;
    ChangeOffsetsWithAdditionalVelocityPrivate(
        offsetsVelocity,
        winrt::float2::zero() /*anticipatedOffsetsChange*/,
        inertiaDecayRate,
        InteractionTrackerAsyncOperationTrigger::DirectViewChange,
        &viewChangeCorrelationId);

    return viewChangeCorrelationId;
}

int32_t ScrollPresenter::ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(), zoomFactor);

    return ZoomTo(zoomFactor, centerPoint, nullptr /*options*/);
}

int32_t ScrollPresenter::ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactor);

    int32_t viewChangeCorrelationId;
    ChangeZoomFactorPrivate(
        zoomFactor,
        centerPoint,
        ScrollPresenterViewKind::Absolute,
        options,
        &viewChangeCorrelationId);

    return viewChangeCorrelationId;
}

int32_t ScrollPresenter::ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        zoomFactorDelta);

    return ZoomBy(zoomFactorDelta, centerPoint, nullptr /*options*/);
}

int32_t ScrollPresenter::ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactorDelta);

    int32_t viewChangeCorrelationId;
    ChangeZoomFactorPrivate(
        zoomFactorDelta,
        centerPoint,
        ScrollPresenterViewKind::RelativeToCurrentView,
        options,
        &viewChangeCorrelationId);

    return viewChangeCorrelationId;
}

int32_t ScrollPresenter::AddZoomVelocity(float zoomFactorVelocity, winrt::IReference<winrt::float2> centerPoint, winrt::IReference<float> inertiaDecayRate)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(),
        zoomFactorVelocity);

    int32_t viewChangeCorrelationId;
    ChangeZoomFactorWithAdditionalVelocityPrivate(
        zoomFactorVelocity,
        0.0f /*anticipatedZoomFactorChange*/,
        centerPoint,
        inertiaDecayRate,
        InteractionTrackerAsyncOperationTrigger::DirectViewChange,
        &viewChangeCorrelationId);

    return viewChangeCorrelationId;
}

#pragma endregion

#pragma region IFrameworkElementOverridesHelper

winrt::Size ScrollPresenter::MeasureOverride(winrt::Size const& availableSize)
{
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"availableSize:", availableSize.Width, availableSize.Height);

    m_availableSize = availableSize;

    const double layoutRoundFactor = GetLayoutRoundFactor();

    if (m_layoutRoundFactor != layoutRoundFactor)
    {
        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old layoutRoundFactor:", m_layoutRoundFactor);
        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new layoutRoundFactor:", layoutRoundFactor);

        m_layoutRoundFactor = layoutRoundFactor;
    }

    winrt::Size contentDesiredSize{ 0.0f, 0.0f };
    const winrt::UIElement content = Content();

    if (content)
    {
        // The content is measured with infinity in the directions in which it is not constrained, enabling this ScrollPresenter
        // to be scrollable in those directions.
        winrt::Size contentAvailableSize
        {
            (m_contentOrientation == winrt::ScrollingContentOrientation::Vertical || m_contentOrientation == winrt::ScrollingContentOrientation::None) ?
                availableSize.Width : std::numeric_limits<float>::infinity(),
            (m_contentOrientation == winrt::ScrollingContentOrientation::Horizontal || m_contentOrientation == winrt::ScrollingContentOrientation::None) ?
                availableSize.Height : std::numeric_limits<float>::infinity()
        };

        if (m_contentOrientation != winrt::ScrollingContentOrientation::Both)
        {
            const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

            if (contentAsFE)
            {
                const winrt::Thickness contentMargin = contentAsFE.Margin();

                if (m_contentOrientation == winrt::ScrollingContentOrientation::Vertical || m_contentOrientation == winrt::ScrollingContentOrientation::None)
                {
                    // Even though the content's Width is constrained, take into account the MinWidth, Width and MaxWidth values
                    // potentially set on the content so it is allowed to grow accordingly.
                    contentAvailableSize.Width = static_cast<float>(GetComputedMaxWidth(availableSize.Width, contentAsFE));
                }
                if (m_contentOrientation == winrt::ScrollingContentOrientation::Horizontal || m_contentOrientation == winrt::ScrollingContentOrientation::None)
                {
                    // Even though the content's Height is constrained, take into account the MinHeight, Height and MaxHeight values
                    // potentially set on the content so it is allowed to grow accordingly.
                    contentAvailableSize.Height = static_cast<float>(GetComputedMaxHeight(availableSize.Height, contentAsFE));
                }
            }
        }

        content.Measure(contentAvailableSize);
        contentDesiredSize = content.DesiredSize();
    }

    // The framework determines that this ScrollPresenter is scrollable when unclippedDesiredSize.Width/Height > desiredSize.Width/Height
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"contentDesiredSize:", contentDesiredSize.Width, contentDesiredSize.Height);

    return contentDesiredSize;
}

winrt::Size ScrollPresenter::ArrangeOverride(winrt::Size const& finalSize)
{
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"finalSize", finalSize.Width, finalSize.Height);

    const bool layoutRoundFactorChanged = m_layoutRoundFactor != GetLayoutRoundFactor();
    const winrt::UIElement content = Content();
    winrt::Rect finalContentRect{};

    // Possible cases:
    // 1. m_availableSize is infinite, the ScrollPresenter is not constrained and takes its Content DesiredSize.
    //    viewport thus is finalSize.
    // 2. m_availableSize > finalSize, the ScrollPresenter is constrained and its Content is smaller than the available size.
    //    No matter the ScrollPresenter's alignment, it does not grow larger than finalSize. viewport is finalSize again.
    // 3. m_availableSize <= finalSize, the ScrollPresenter is constrained and its Content is larger than or equal to
    //    the available size. viewport is the smaller & constrained m_availableSize.
    const winrt::Size viewport =
    {
        std::min(finalSize.Width, m_availableSize.Width),
        std::min(finalSize.Height, m_availableSize.Height)
    };

    bool renderSizeChanged = false;
    double newUnzoomedExtentWidth = 0.0;
    double newUnzoomedExtentHeight = 0.0;

    if (content)
    {
        float contentLayoutOffsetXDelta = 0.0f;
        float contentLayoutOffsetYDelta = 0.0f;
        bool isAnchoringElementHorizontally = false;
        bool isAnchoringElementVertically = false;
        bool isAnchoringFarEdgeHorizontally = false;
        bool isAnchoringFarEdgeVertically = false;
        const winrt::Size oldRenderSize = content.RenderSize();
        winrt::Size contentArrangeSize = content.DesiredSize();

        const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

        const winrt::Thickness contentMargin = [contentAsFE]()
            {
                return contentAsFE ? contentAsFE.Margin() : winrt::Thickness{ 0 };
            }();

        const bool wasContentArrangeWidthStretched = [contentAsFE, contentArrangeSize, viewport]()
            {
                return contentAsFE &&
                    contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch &&
                    isnan(contentAsFE.Width()) &&
                    contentArrangeSize.Width < viewport.Width;
            }();

        const bool wasContentArrangeHeightStretched = [contentAsFE, contentArrangeSize, viewport]()
            {
                return contentAsFE &&
                    contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch &&
                    isnan(contentAsFE.Height()) &&
                    contentArrangeSize.Height < viewport.Height;
            }();

        if (wasContentArrangeWidthStretched)
        {
            // Allow the content to stretch up to the larger viewport width.
            contentArrangeSize.Width = viewport.Width;
        }

        if (wasContentArrangeHeightStretched)
        {
            // Allow the content to stretch up to the larger viewport height.
            contentArrangeSize.Height = viewport.Height;
        }

        finalContentRect =
        {
            m_contentLayoutOffsetX,
            m_contentLayoutOffsetY,
            contentArrangeSize.Width,
            contentArrangeSize.Height
        };

        IsAnchoring(&isAnchoringElementHorizontally, &isAnchoringElementVertically, &isAnchoringFarEdgeHorizontally, &isAnchoringFarEdgeVertically);

        MUX_ASSERT(!(isAnchoringElementHorizontally && isAnchoringFarEdgeHorizontally));
        MUX_ASSERT(!(isAnchoringElementVertically && isAnchoringFarEdgeVertically));

        if (isAnchoringElementHorizontally || isAnchoringElementVertically || isAnchoringFarEdgeHorizontally || isAnchoringFarEdgeVertically)
        {
            MUX_ASSERT(m_interactionTracker);

            winrt::Size preArrangeViewportToElementAnchorPointsDistance{ FloatUtil::NaN, FloatUtil::NaN };

            if (isAnchoringElementHorizontally || isAnchoringElementVertically)
            {
                EnsureAnchorElementSelection();
                preArrangeViewportToElementAnchorPointsDistance = ComputeViewportToElementAnchorPointsDistance(
                    m_viewportWidth,
                    m_viewportHeight,
                    true /*isForPreArrange*/);
            }
            else
            {
                ResetAnchorElement();
            }

            contentArrangeSize = ArrangeContent(
                content,
                contentMargin,
                finalContentRect,
                wasContentArrangeWidthStretched,
                wasContentArrangeHeightStretched);

            if (!isnan(preArrangeViewportToElementAnchorPointsDistance.Width) || !isnan(preArrangeViewportToElementAnchorPointsDistance.Height))
            {
                // Using the new viewport sizes to handle the cases where an adjustment needs to be performed because of a ScrollPresenter size change.
                const winrt::Size postArrangeViewportToElementAnchorPointsDistance = ComputeViewportToElementAnchorPointsDistance(
                    viewport.Width /*viewportWidth*/,
                    viewport.Height /*viewportHeight*/,
                    false /*isForPreArrange*/);

                if (isAnchoringElementHorizontally &&
                    !isnan(preArrangeViewportToElementAnchorPointsDistance.Width) &&
                    !isnan(postArrangeViewportToElementAnchorPointsDistance.Width) &&
                    preArrangeViewportToElementAnchorPointsDistance.Width != postArrangeViewportToElementAnchorPointsDistance.Width)
                {
                    // Perform horizontal offset adjustment due to element anchoring
                    contentLayoutOffsetXDelta = ComputeContentLayoutOffsetDelta(
                        ScrollPresenterDimension::HorizontalScroll,
                        postArrangeViewportToElementAnchorPointsDistance.Width - preArrangeViewportToElementAnchorPointsDistance.Width /*unzoomedDelta*/);
                }

                if (isAnchoringElementVertically &&
                    !isnan(preArrangeViewportToElementAnchorPointsDistance.Height) &&
                    !isnan(postArrangeViewportToElementAnchorPointsDistance.Height) &&
                    preArrangeViewportToElementAnchorPointsDistance.Height != postArrangeViewportToElementAnchorPointsDistance.Height)
                {
                    // Perform vertical offset adjustment due to element anchoring
                    contentLayoutOffsetYDelta = ComputeContentLayoutOffsetDelta(
                        ScrollPresenterDimension::VerticalScroll,
                        postArrangeViewportToElementAnchorPointsDistance.Height - preArrangeViewportToElementAnchorPointsDistance.Height /*unzoomedDelta*/);
                }
            }
        }
        else
        {
            ResetAnchorElement();

            contentArrangeSize = ArrangeContent(
                content,
                contentMargin,
                finalContentRect,
                wasContentArrangeWidthStretched,
                wasContentArrangeHeightStretched);
        }

        newUnzoomedExtentWidth = contentArrangeSize.Width;
        newUnzoomedExtentHeight = contentArrangeSize.Height;

        double maxUnzoomedExtentWidth = std::numeric_limits<double>::infinity();
        double maxUnzoomedExtentHeight = std::numeric_limits<double>::infinity();

        if (contentAsFE)
        {
            // Determine the maximum size directly set on the content, if any.
            maxUnzoomedExtentWidth = GetComputedMaxWidth(maxUnzoomedExtentWidth, contentAsFE);
            maxUnzoomedExtentHeight = GetComputedMaxHeight(maxUnzoomedExtentHeight, contentAsFE);
        }

        // Take into account the actual resulting rendering size, in case it's larger than the desired size.
        // But the extent must not exceed the size explicitly set on the content, if any.
        newUnzoomedExtentWidth = std::max(
            newUnzoomedExtentWidth,
            std::max(0.0, content.RenderSize().Width + contentMargin.Left + contentMargin.Right));
        newUnzoomedExtentWidth = std::min(
            newUnzoomedExtentWidth,
            maxUnzoomedExtentWidth);

        newUnzoomedExtentHeight = std::max(
            newUnzoomedExtentHeight,
            std::max(0.0, content.RenderSize().Height + contentMargin.Top + contentMargin.Bottom));
        newUnzoomedExtentHeight = std::min(
            newUnzoomedExtentHeight,
            maxUnzoomedExtentHeight);

        if (isAnchoringFarEdgeHorizontally)
        {
            float unzoomedDelta = 0.0f;

            if (newUnzoomedExtentWidth > m_unzoomedExtentWidth ||                                  // ExtentWidth grew
                m_zoomedHorizontalOffset + m_viewportWidth > m_zoomFactor * m_unzoomedExtentWidth) // ExtentWidth shrank while overpanning
            {
                // Perform horizontal offset adjustment due to edge anchoring
                unzoomedDelta = static_cast<float>(newUnzoomedExtentWidth - m_unzoomedExtentWidth);
            }

            if (static_cast<float>(m_viewportWidth) > viewport.Width)
            {
                // Viewport width shrank: Perform horizontal offset adjustment due to edge anchoring
                unzoomedDelta += (static_cast<float>(m_viewportWidth) - viewport.Width) / m_zoomFactor;
            }

            if (unzoomedDelta != 0.0f)
            {
                MUX_ASSERT(contentLayoutOffsetXDelta == 0.0f);
                contentLayoutOffsetXDelta = ComputeContentLayoutOffsetDelta(ScrollPresenterDimension::HorizontalScroll, unzoomedDelta);
            }
        }

        if (isAnchoringFarEdgeVertically)
        {
            float unzoomedDelta = 0.0f;

            if (newUnzoomedExtentHeight > m_unzoomedExtentHeight ||                                // ExtentHeight grew
                m_zoomedVerticalOffset + m_viewportHeight > m_zoomFactor * m_unzoomedExtentHeight) // ExtentHeight shrank while overpanning
            {
                // Perform vertical offset adjustment due to edge anchoring
                unzoomedDelta = static_cast<float>(newUnzoomedExtentHeight - m_unzoomedExtentHeight);
            }

            if (static_cast<float>(m_viewportHeight) > viewport.Height)
            {
                // Viewport height shrank: Perform vertical offset adjustment due to edge anchoring
                unzoomedDelta += (static_cast<float>(m_viewportHeight) - viewport.Height) / m_zoomFactor;
            }

            if (unzoomedDelta != 0.0f)
            {
                MUX_ASSERT(contentLayoutOffsetYDelta == 0.0f);
                contentLayoutOffsetYDelta = ComputeContentLayoutOffsetDelta(ScrollPresenterDimension::VerticalScroll, unzoomedDelta);
            }
        }

        if (contentLayoutOffsetXDelta != 0.0f || contentLayoutOffsetYDelta != 0.0f)
        {
            const winrt::Rect contentRectWithDelta =
            {
                m_contentLayoutOffsetX + contentLayoutOffsetXDelta,
                m_contentLayoutOffsetY + contentLayoutOffsetYDelta,
                contentArrangeSize.Width,
                contentArrangeSize.Height
            };

            SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"content Arrange", TypeLogging::RectToString(contentRectWithDelta).c_str());

            content.Arrange(contentRectWithDelta);

            if (contentLayoutOffsetXDelta != 0.0f)
            {
                m_contentLayoutOffsetX += contentLayoutOffsetXDelta;
                UpdateOffset(ScrollPresenterDimension::HorizontalScroll, m_zoomedHorizontalOffset - contentLayoutOffsetXDelta);
                OnContentLayoutOffsetChanged(ScrollPresenterDimension::HorizontalScroll);
            }

            if (contentLayoutOffsetYDelta != 0.0f)
            {
                m_contentLayoutOffsetY += contentLayoutOffsetYDelta;
                UpdateOffset(ScrollPresenterDimension::VerticalScroll, m_zoomedVerticalOffset - contentLayoutOffsetYDelta);
                OnContentLayoutOffsetChanged(ScrollPresenterDimension::VerticalScroll);
            }

            OnViewChanged(contentLayoutOffsetXDelta != 0.0f /*horizontalOffsetChanged*/, contentLayoutOffsetYDelta != 0.0f /*verticalOffsetChanged*/);
        }

        renderSizeChanged = content.RenderSize() != oldRenderSize;
    }

    // Set a rectangular clip on this ScrollPresenter the same size as the arrange
    // rectangle so the content does not render beyond it.
    auto rectangleGeometry = Clip().as<winrt::RectangleGeometry>();

    if (!rectangleGeometry)
    {
        // Ensure that this ScrollPresenter has a rectangular clip.
        winrt::RectangleGeometry newRectangleGeometry;
        Clip(newRectangleGeometry);

        rectangleGeometry = newRectangleGeometry;
    }

    const winrt::Rect newClipRect{ 0.0f, 0.0f, viewport.Width, viewport.Height };
    rectangleGeometry.Rect(newClipRect);

    if (layoutRoundFactorChanged)
    {
        // The global scale factor has changed since the last measure pass. Do not record the viewport size and extent based on the old
        // scale factor. Instead, trigger new measure & arrange passes which will provide the new precise availableSize, result in the
        // correct viewport and content sizes and push them to the potential scroll controllers. Calling UpdateUnzoomedExtentAndViewport
        // with slightly incorrect sizes could result in wrong IScrollController::CanScroll evaluations and layout cycles.
        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"layoutRoundFactor changed since last measure pass.");

        InvalidateMeasure();
    }
    else
    {
        UpdateUnzoomedExtentAndViewport(
            renderSizeChanged,
            newUnzoomedExtentWidth  /*unzoomedExtentWidth*/,
            newUnzoomedExtentHeight /*unzoomedExtentHeight*/,
            viewport.Width          /*viewportWidth*/,
            viewport.Height         /*viewportHeight*/);
    }

    m_isAnchorElementDirty = true;
    return viewport;
}

#pragma endregion

#pragma region IInteractionTrackerOwner

void ScrollPresenter::CustomAnimationStateEntered(
    const winrt::InteractionTrackerCustomAnimationStateEnteredArgs& args)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());

    UpdateState(winrt::ScrollingInteractionState::Animation);
}

void ScrollPresenter::IdleStateEntered(
    const winrt::InteractionTrackerIdleStateEnteredArgs& args)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());

    UpdateState(winrt::ScrollingInteractionState::Idle);

    if (!m_interactionTrackerAsyncOperations.empty())
    {
        const int32_t requestId = args.RequestId();

        // Complete all operations recorded through ChangeOffsetsPrivate/ChangeOffsetsWithAdditionalVelocityPrivate
        // and ChangeZoomFactorPrivate/ChangeZoomFactorWithAdditionalVelocityPrivate calls.
        if (requestId != 0)
        {
            CompleteInteractionTrackerOperations(
                requestId,
                ScrollPresenterViewChangeResult::Completed   /*operationResult*/,
                ScrollPresenterViewChangeResult::Completed   /*priorNonAnimatedOperationsResult*/,
                ScrollPresenterViewChangeResult::Interrupted /*priorAnimatedOperationsResult*/,
                true  /*completeNonAnimatedOperation*/,
                true  /*completeAnimatedOperation*/,
                true  /*completePriorNonAnimatedOperations*/,
                true  /*completePriorAnimatedOperations*/);
        }
    }

    // Check if resting position corresponds to a non-unique mandatory snap point, for the three dimensions
    UpdateSnapPointsIgnoredValue(&m_sortedConsolidatedHorizontalSnapPoints, ScrollPresenterDimension::HorizontalScroll);
    UpdateSnapPointsIgnoredValue(&m_sortedConsolidatedVerticalSnapPoints, ScrollPresenterDimension::VerticalScroll);
    UpdateSnapPointsIgnoredValue(&m_sortedConsolidatedZoomSnapPoints, ScrollPresenterDimension::ZoomFactor);

    // Stop Translation and Scale animations if needed, to trigger rasterization of Content & avoid fuzzy text rendering for instance.
    StopTranslationAndZoomFactorExpressionAnimations();
}

void ScrollPresenter::InertiaStateEntered(
    const winrt::InteractionTrackerInertiaStateEnteredArgs& args)
{
    winrt::IReference<winrt::float3> modifiedRestingPosition = args.ModifiedRestingPosition();
    const winrt::float3 naturalRestingPosition = args.NaturalRestingPosition();
    winrt::IReference<float> modifiedRestingScale = args.ModifiedRestingScale();
    const float naturalRestingScale = args.NaturalRestingScale();
    const bool isTracingEnabled = IsScrollPresenterTracingEnabled() || ScrollPresenterTrace::s_IsDebugOutputEnabled || ScrollPresenterTrace::s_IsVerboseDebugOutputEnabled;
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationFromRequestId(args.RequestId());

    if (isTracingEnabled)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_PTR_INT, METH_NAME, this, interactionTrackerAsyncOperation.get(), args.RequestId());

        const winrt::float3 positionVelocity = args.PositionVelocityInPixelsPerSecond();
        const float scaleVelocity = args.ScaleVelocityInPercentPerSecond();

        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
            TypeLogging::Float2ToString(winrt::float2{ positionVelocity.x, positionVelocity.y }).c_str(),
            scaleVelocity);

        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
            TypeLogging::Float2ToString(winrt::float2{ naturalRestingPosition.x, naturalRestingPosition.y }).c_str(),
            naturalRestingScale);

        if (modifiedRestingPosition)
        {
            const winrt::float3 endOfInertiaPosition = modifiedRestingPosition.Value();
            SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this,
                TypeLogging::Float2ToString(winrt::float2{ endOfInertiaPosition.x, endOfInertiaPosition.y }).c_str());
        }

        if (modifiedRestingScale)
        {
            SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_FLT, METH_NAME, this,
                modifiedRestingScale.Value());
        }

        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.IsInertiaFromImpulse());
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.IsFromBinding());
    }

    // Record the end-of-inertia view for this inertial phase. It may be needed for
    // custom pointer wheel processing.

    if (modifiedRestingPosition)
    {
        const winrt::float3 endOfInertiaPosition = modifiedRestingPosition.Value();
        m_endOfInertiaPosition = { endOfInertiaPosition.x, endOfInertiaPosition.y };
    }
    else
    {
        m_endOfInertiaPosition = { naturalRestingPosition.x, naturalRestingPosition.y };
    }

    if (modifiedRestingScale)
    {
        m_endOfInertiaZoomFactor = modifiedRestingScale.Value();
    }
    else
    {
        m_endOfInertiaZoomFactor = naturalRestingScale;
    }

    if (isTracingEnabled)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
            TypeLogging::Float2ToString(m_endOfInertiaPosition).c_str(),
            m_endOfInertiaZoomFactor);
    }

    if (interactionTrackerAsyncOperation)
    {
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();

        if (viewChangeBase && interactionTrackerAsyncOperation->GetOperationType() == InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity)
        {
            std::shared_ptr<OffsetsChangeWithAdditionalVelocity> offsetsChangeWithAdditionalVelocity = std::reinterpret_pointer_cast<OffsetsChangeWithAdditionalVelocity>(viewChangeBase);

            if (offsetsChangeWithAdditionalVelocity)
            {
                offsetsChangeWithAdditionalVelocity->AnticipatedOffsetsChange(winrt::float2::zero());
            }
        }
    }

    UpdateState(winrt::ScrollingInteractionState::Inertia);
}

void ScrollPresenter::InteractingStateEntered(
    const winrt::InteractionTrackerInteractingStateEnteredArgs& args)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());

    if (m_state == winrt::ScrollingInteractionState::Inertia)
    {
        // Restore the default 0.95 position inertia decay rate since it may have been overridden by an offset change with additional velocity.
        ResetOffsetsInertiaDecayRate();

        // Restore the default 0.985 zoomFactor inertia decay rate since it may have been overridden by a zoomFactor change with additional velocity.
        ResetZoomFactorInertiaDecayRate();
    }

    UpdateState(winrt::ScrollingInteractionState::Interaction);

    if (!m_interactionTrackerAsyncOperations.empty())
    {
        // Complete all operations recorded through ChangeOffsetsPrivate/ChangeOffsetsWithAdditionalVelocityPrivate
        // and ChangeZoomFactorPrivate/ChangeZoomFactorWithAdditionalVelocityPrivate calls.
        CompleteInteractionTrackerOperations(
            -1 /*requestId*/,
            ScrollPresenterViewChangeResult::Interrupted /*operationResult*/,
            ScrollPresenterViewChangeResult::Completed   /*priorNonAnimatedOperationsResult*/,
            ScrollPresenterViewChangeResult::Interrupted /*priorAnimatedOperationsResult*/,
            true  /*completeNonAnimatedOperation*/,
            true  /*completeAnimatedOperation*/,
            true  /*completePriorNonAnimatedOperations*/,
            true  /*completePriorAnimatedOperations*/);
    }
}

void ScrollPresenter::RequestIgnored(
    const winrt::InteractionTrackerRequestIgnoredArgs& args)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());

    if (!m_interactionTrackerAsyncOperations.empty())
    {
        // Complete this request alone.
        CompleteInteractionTrackerOperations(
            args.RequestId(),
            ScrollPresenterViewChangeResult::Ignored /*operationResult*/,
            ScrollPresenterViewChangeResult::Ignored /*unused priorNonAnimatedOperationsResult*/,
            ScrollPresenterViewChangeResult::Ignored /*unused priorAnimatedOperationsResult*/,
            true  /*completeNonAnimatedOperation*/,
            true  /*completeAnimatedOperation*/,
            false /*completePriorNonAnimatedOperations*/,
            false /*completePriorAnimatedOperations*/);
    }
}

void ScrollPresenter::ValuesChanged(
    const winrt::InteractionTrackerValuesChangedArgs& args)
{
    const bool isScrollPresenterTracingEnabled = IsScrollPresenterTracingEnabled();

#ifdef DBG
    if (isScrollPresenterTracingEnabled || ScrollPresenterTrace::s_IsDebugOutputEnabled || ScrollPresenterTrace::s_IsVerboseDebugOutputEnabled)
    {
        SCROLLPRESENTER_TRACE_INFO_ENABLED(isScrollPresenterTracingEnabled /*includeTraceLogging*/, *this, L"%s[0x%p](RequestId: %d, View: %f, %f, %f)\n",
            METH_NAME, this, args.RequestId(), args.Position().x, args.Position().y, args.Scale());
    }
#endif

    const int requestId = args.RequestId();

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationFromRequestId(requestId);

    const bool isRightToLeftDirection = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const double oldZoomedHorizontalOffset = m_zoomedHorizontalOffset;
    const double oldZoomedVerticalOffset = m_zoomedVerticalOffset;
    const float oldZoomFactor = m_zoomFactor;
    winrt::float2 minPosition{};
    winrt::float2 maxPosition{};

    m_zoomFactor = args.Scale();

    ComputeMinMaxPositions(m_zoomFactor, &minPosition, isRightToLeftDirection ? &maxPosition : nullptr);

    if (isRightToLeftDirection)
    {
        UpdateOffset(ScrollPresenterDimension::HorizontalScroll, static_cast<double>(maxPosition.x) - args.Position().x);
    }
    else
    {
        UpdateOffset(ScrollPresenterDimension::HorizontalScroll, static_cast<double>(args.Position().x) - minPosition.x);
    }

    UpdateOffset(ScrollPresenterDimension::VerticalScroll, static_cast<double>(args.Position().y) - minPosition.y);

    if (oldZoomFactor != m_zoomFactor || oldZoomedHorizontalOffset != m_zoomedHorizontalOffset || oldZoomedVerticalOffset != m_zoomedVerticalOffset)
    {
        OnViewChanged(oldZoomedHorizontalOffset != m_zoomedHorizontalOffset /*horizontalOffsetChanged*/,
                      oldZoomedVerticalOffset != m_zoomedVerticalOffset /*verticalOffsetChanged*/);
    }

    TraceLoggingProviderWrite(
        XamlTelemetryLogging, "ScrollPresenter_ValuesChanged",
        TraceLoggingFloat64(m_zoomedHorizontalOffset, "HorizontalOffset"),
        TraceLoggingFloat64(m_zoomedVerticalOffset, "VerticalOffset"),
        TraceLoggingFloat32(m_zoomFactor, "ZoomFactor"),
        TraceLoggingFloat64(oldZoomedHorizontalOffset, "OldHorizontalOffset"),
        TraceLoggingFloat64(oldZoomedVerticalOffset, "OldVerticalOffset"),
        TraceLoggingFloat32(oldZoomFactor, "OldZoomFactor"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    if (requestId != 0 && !m_interactionTrackerAsyncOperations.empty())
    {
        CompleteInteractionTrackerOperations(
            requestId,
            ScrollPresenterViewChangeResult::Completed   /*operationResult*/,
            ScrollPresenterViewChangeResult::Completed   /*priorNonAnimatedOperationsResult*/,
            ScrollPresenterViewChangeResult::Interrupted /*priorAnimatedOperationsResult*/,
            true  /*completeNonAnimatedOperation*/,
            false /*completeAnimatedOperation*/,
            true  /*completePriorNonAnimatedOperations*/,
            true  /*completePriorAnimatedOperations*/);
    }
}

#pragma endregion

// Returns the size used to arrange the provided ScrollPresenter content.
winrt::Size ScrollPresenter::ArrangeContent(
    const winrt::UIElement& content,
    const winrt::Thickness& contentMargin,
    winrt::Rect& finalContentRect,
    bool wasContentArrangeWidthStretched,
    bool wasContentArrangeHeightStretched)
{
    MUX_ASSERT(content);

    winrt::Size contentArrangeSize =
    {
        finalContentRect.Width,
        finalContentRect.Height
    };

    content.Arrange(finalContentRect);

    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"content Arrange", TypeLogging::RectToString(finalContentRect).c_str());
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"wasContentArrangeWidthStretched", wasContentArrangeWidthStretched);
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"wasContentArrangeHeightStretched", wasContentArrangeHeightStretched);
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"content RenderSize", content.RenderSize().Width, content.RenderSize().Height);

    if (wasContentArrangeWidthStretched || wasContentArrangeHeightStretched)
    {
        bool reArrangeNeeded = false;
        const auto renderWidth = content.RenderSize().Width;
        const auto renderHeight = content.RenderSize().Height;
        const auto marginWidth = static_cast<float>(contentMargin.Left + contentMargin.Right);
        const auto marginHeight = static_cast<float>(contentMargin.Top + contentMargin.Bottom);
        const auto scaleFactorRounding = 0.5f / static_cast<float>(XamlRoot().RasterizationScale());

        if (wasContentArrangeWidthStretched &&
            renderWidth > 0.0f &&
            renderWidth + marginWidth < finalContentRect.Width * (1.0f - std::numeric_limits<float>::epsilon()) - scaleFactorRounding)
        {
            // Content stretched partially horizontally.
            contentArrangeSize.Width = finalContentRect.Width = renderWidth + marginWidth;
            reArrangeNeeded = true;
        }

        if (wasContentArrangeHeightStretched &&
            renderHeight > 0.0f &&
            renderHeight + marginHeight < finalContentRect.Height * (1.0f - std::numeric_limits<float>::epsilon()) - scaleFactorRounding)
        {
            // Content stretched partially vertically.
            contentArrangeSize.Height = finalContentRect.Height = renderHeight + marginHeight;
            reArrangeNeeded = true;
        }

        if (reArrangeNeeded)
        {
            // Re-arrange the content using the partially stretched size.
            SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"content re-Arrange", TypeLogging::RectToString(finalContentRect).c_str());

            content.Arrange(finalContentRect);
        }
    }

    return contentArrangeSize;
}

// Used to perform a flickerless change to the Content's XAML Layout Offset. The InteractionTracker's Position is unaffected, but its Min/MaxPosition expressions
// and the ScrollPresenter HorizontalOffset/VerticalOffset property are updated accordingly once the change is incorporated into the XAML layout engine.
float ScrollPresenter::ComputeContentLayoutOffsetDelta(ScrollPresenterDimension dimension, float unzoomedDelta) const
{
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);

    float zoomedDelta = unzoomedDelta * m_zoomFactor;

    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, dimension == ScrollPresenterDimension::HorizontalScroll ? L"HorizontalScroll" : L"VerticalScroll");
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"zoomedDelta", zoomedDelta);

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"m_zoomedHorizontalOffset", m_zoomedHorizontalOffset);

        if (zoomedDelta < 0.0f && -zoomedDelta > m_zoomedHorizontalOffset)
        {
            // Do not let m_zoomedHorizontalOffset step into negative territory.
            zoomedDelta = static_cast<float>(-m_zoomedHorizontalOffset);
        }
    }
    else
    {
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"m_zoomedVerticalOffset", m_zoomedVerticalOffset);

        if (zoomedDelta < 0.0f && -zoomedDelta > m_zoomedVerticalOffset)
        {
            // Do not let m_zoomedVerticalOffset step into negative territory.
            zoomedDelta = static_cast<float>(-m_zoomedVerticalOffset);
        }
    }

    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"returned value", -zoomedDelta);

    return -zoomedDelta;
}

float ScrollPresenter::ComputeEndOfInertiaZoomFactor() const
{
    if (m_state == winrt::ScrollingInteractionState::Inertia)
    {
        return std::clamp(m_endOfInertiaZoomFactor, m_interactionTracker.MinScale(), m_interactionTracker.MaxScale());
    }
    else
    {
        return m_zoomFactor;
    }
}

#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
winrt::float2 ScrollPresenter::ComputeEndOfInertiaPosition()
{
    if (m_state == winrt::ScrollingInteractionState::Inertia)
    {
        const float endOfInertiaZoomFactor = ComputeEndOfInertiaZoomFactor();
        winrt::float2 minPosition{};
        winrt::float2 maxPosition{};
        winrt::float2 endOfInertiaPosition = m_endOfInertiaPosition;

        ComputeMinMaxPositions(endOfInertiaZoomFactor, &minPosition, &maxPosition);

        endOfInertiaPosition = max(endOfInertiaPosition, minPosition);
        endOfInertiaPosition = min(endOfInertiaPosition, maxPosition);

        return endOfInertiaPosition;
    }
    else
    {
        return ComputePositionFromOffsets(m_zoomedHorizontalOffset, m_zoomedVerticalOffset);
    }
}
#endif // ScrollPresenterViewKind_RelativeToEndOfInertiaView

// Returns zoomed vectors corresponding to InteractionTracker.MinPosition and InteractionTracker.MaxPosition
// Determines the min and max positions of the ScrollPresenter.Content based on its size and alignment, and the ScrollPresenter size.
void ScrollPresenter::ComputeMinMaxPositions(float zoomFactor, _Out_opt_ winrt::float2* minPosition, _Out_opt_ winrt::float2* maxPosition)
{
    MUX_ASSERT(minPosition || maxPosition);

    if (minPosition)
    {
        *minPosition = winrt::float2::zero();
    }

    if (maxPosition)
    {
        *maxPosition = winrt::float2::zero();
    }

    const winrt::UIElement content = Content();

    if (!content)
    {
        return;
    }

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (!contentAsFE)
    {
        return;
    }

    const bool isRightToLeftDirection = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const winrt::Visual scrollPresenterVisual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    float minPosX = 0.0f;
    float minPosY = 0.0f;
    float maxPosX = 0.0f;
    float maxPosY = 0.0f;
    const float extentWidth = static_cast<float>(m_unzoomedExtentWidth);
    const float extentHeight = static_cast<float>(m_unzoomedExtentHeight);

    if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
        contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
    {
        const float scrollableWidth = extentWidth * zoomFactor - scrollPresenterVisual.Size().x;

        if (minPosition || (isRightToLeftDirection && maxPosition))
        {
            // When the zoomed content is smaller than the viewport, scrollableWidth < 0, minPosX is scrollableWidth / 2 so it is centered at idle.
            // When the zoomed content is larger than the viewport, scrollableWidth > 0, minPosX is 0.
            minPosX = std::min(0.0f, scrollableWidth / 2.0f);
        }

        if (maxPosition || (isRightToLeftDirection && minPosition))
        {
            // When the zoomed content is smaller than the viewport, scrollableWidth < 0, maxPosX is scrollableWidth / 2 so it is centered at idle.
            // When the zoomed content is larger than the viewport, scrollableWidth > 0, maxPosX is scrollableWidth.
            maxPosX = scrollableWidth;
            if (maxPosX < 0.0f)
            {
                maxPosX /= 2.0f;
            }
        }
    }
    else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
    {
        const float scrollableWidth = extentWidth * zoomFactor - scrollPresenterVisual.Size().x;

        if (minPosition || (isRightToLeftDirection && maxPosition))
        {
            // When the zoomed content is smaller than the viewport, scrollableWidth < 0, minPosX is scrollableWidth so it is right-aligned at idle.
            // When the zoomed content is larger than the viewport, scrollableWidth > 0, minPosX is 0.
            minPosX = std::min(0.0f, scrollableWidth);
        }

        if (maxPosition || (isRightToLeftDirection && minPosition))
        {
            // When the zoomed content is smaller than the viewport, scrollableWidth < 0, maxPosX is -scrollableWidth so it is right-aligned at idle.
            // When the zoomed content is larger than the viewport, scrollableWidth > 0, maxPosX is scrollableWidth.
            maxPosX = scrollableWidth;
            if (maxPosX < 0.0f)
            {
                maxPosX *= -1.0f;
            }
        }
    }

    if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
        contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
    {
        const float scrollableHeight = extentHeight * zoomFactor - scrollPresenterVisual.Size().y;

        if (minPosition || (isRightToLeftDirection && maxPosition))
        {
            // When the zoomed content is smaller than the viewport, scrollableHeight < 0, minPosY is scrollableHeight / 2 so it is centered at idle.
            // When the zoomed content is larger than the viewport, scrollableHeight > 0, minPosY is 0.
            minPosY = std::min(0.0f, scrollableHeight / 2.0f);
        }

        if (maxPosition || (isRightToLeftDirection && minPosition))
        {
            // When the zoomed content is smaller than the viewport, scrollableHeight < 0, maxPosY is scrollableHeight / 2 so it is centered at idle.
            // When the zoomed content is larger than the viewport, scrollableHeight > 0, maxPosY is scrollableHeight.
            maxPosY = scrollableHeight;
            if (maxPosY < 0.0f)
            {
                maxPosY /= 2.0f;
            }
        }
    }
    else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
    {
        const float scrollableHeight = extentHeight * zoomFactor - scrollPresenterVisual.Size().y;

        if (minPosition || (isRightToLeftDirection && maxPosition))
        {
            // When the zoomed content is smaller than the viewport, scrollableHeight < 0, minPosY is scrollableHeight so it is bottom-aligned at idle.
            // When the zoomed content is larger than the viewport, scrollableHeight > 0, minPosY is 0.
            minPosY = std::min(0.0f, scrollableHeight);
        }

        if (maxPosition || (isRightToLeftDirection && minPosition))
        {
            // When the zoomed content is smaller than the viewport, scrollableHeight < 0, maxPosY is -scrollableHeight so it is bottom-aligned at idle.
            // When the zoomed content is larger than the viewport, scrollableHeight > 0, maxPosY is scrollableHeight.
            maxPosY = scrollableHeight;
            if (maxPosY < 0.0f)
            {
                maxPosY *= -1.0f;
            }
        }
    }

    if (minPosition)
    {
        if (isRightToLeftDirection)
        {
            *minPosition = winrt::float2(-maxPosX - m_contentLayoutOffsetX, minPosY + m_contentLayoutOffsetY);
        }
        else
        {
            *minPosition = winrt::float2(minPosX + m_contentLayoutOffsetX, minPosY + m_contentLayoutOffsetY);
        }

#ifdef DBG
        // Allow ScrollPresenterTestHooks to override the returned value.
        if (!isnan(m_minPositionOverrideDbg.x) && !isnan(m_minPositionOverrideDbg.y))
        {
            *minPosition = m_minPositionOverrideDbg;
        }
#endif // DBG
    }

    if (maxPosition)
    {
        if (isRightToLeftDirection)
        {
            *maxPosition = winrt::float2(-minPosX - m_contentLayoutOffsetX, maxPosY + m_contentLayoutOffsetY);
        }
        else
        {
            *maxPosition = winrt::float2(maxPosX + m_contentLayoutOffsetX, maxPosY + m_contentLayoutOffsetY);
        }

#ifdef DBG
        // Allow ScrollPresenterTestHooks to override the returned value.
        if (!isnan(m_maxPositionOverrideDbg.x) && !isnan(m_maxPositionOverrideDbg.y))
        {
            *maxPosition = m_maxPositionOverrideDbg;
        }
#endif // DBG
    }
}

// Returns an InteractionTracker Position based on the provided offsets
winrt::float2 ScrollPresenter::ComputePositionFromOffsets(double zoomedHorizontalOffset, double zoomedVerticalOffset)
{
    const bool isRightToLeftDirection = FlowDirection() == winrt::FlowDirection::RightToLeft;
    winrt::float2 minPosition{};
    winrt::float2 maxPosition{};

    ComputeMinMaxPositions(m_zoomFactor, &minPosition, isRightToLeftDirection ? &maxPosition : nullptr);

    if (isRightToLeftDirection)
    {
        return winrt::float2(static_cast<float>(maxPosition.x - zoomedHorizontalOffset), static_cast<float>(zoomedVerticalOffset + minPosition.y));
    }
    else
    {
        return winrt::float2(static_cast<float>(zoomedHorizontalOffset + minPosition.x), static_cast<float>(zoomedVerticalOffset + minPosition.y));
    }
}

// Evaluate what the value will be once the snap points have been applied.
template <typename T>
double ScrollPresenter::ComputeValueAfterSnapPoints(
    double value,
    std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>> const& snapPointsSet)
{
    for (std::shared_ptr<SnapPointWrapper<T>> snapPointWrapper : snapPointsSet)
    {
        if (std::get<0>(snapPointWrapper->ActualApplicableZone()) <= value &&
            std::get<1>(snapPointWrapper->ActualApplicableZone()) >= value)
        {
            return snapPointWrapper->Evaluate(static_cast<float>(value));
        }
    }
    return value;
}

// Called by ScrollPresenter::OnBringIntoViewRequestedHandler to compute the target bring-into-view offsets
// based on the provided BringIntoViewRequestedEventArgs instance.
// The resulting offsets are later updated by ScrollPresenter::ComputeBringIntoViewUpdatedTargetOffsets below.
void ScrollPresenter::ComputeBringIntoViewTargetOffsetsFromRequestEventArgs(
    const winrt::UIElement& content,
    const winrt::ScrollingSnapPointsMode& snapPointsMode,
    const winrt::BringIntoViewRequestedEventArgs& requestEventArgs,
    _Out_ double* targetZoomedHorizontalOffset,
    _Out_ double* targetZoomedVerticalOffset,
    _Out_ double* appliedOffsetX,
    _Out_ double* appliedOffsetY,
    _Out_ winrt::Rect* targetRect)
{
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, L"%s[0x%p](H/V AlignmentRatio:%lf,%lf, H/V Offset:%f,%f, ElementRect:%s, Element:0x%p)\n",
        METH_NAME, this,
        requestEventArgs.HorizontalAlignmentRatio(), requestEventArgs.VerticalAlignmentRatio(),
        requestEventArgs.HorizontalOffset(), requestEventArgs.VerticalOffset(),
        TypeLogging::RectToString(requestEventArgs.TargetRect()).c_str(), requestEventArgs.TargetElement());

    ComputeBringIntoViewTargetOffsets(
        content,
        requestEventArgs.TargetElement(),
        requestEventArgs.TargetRect(),
        snapPointsMode,
        requestEventArgs.HorizontalAlignmentRatio(),
        requestEventArgs.VerticalAlignmentRatio(),
        requestEventArgs.HorizontalOffset(),
        requestEventArgs.VerticalOffset(),
        targetZoomedHorizontalOffset,
        targetZoomedVerticalOffset,
        appliedOffsetX,
        appliedOffsetY,
        targetRect);
}

// Called by ScrollPresenter::ProcessOffsetsChange to potentially update the target bring-into-view offsets
// just before invoking the InteractionTracker's TryUpdatePosition.
void ScrollPresenter::ComputeBringIntoViewUpdatedTargetOffsets(
    const winrt::UIElement& content,
    const winrt::UIElement& element,
    const winrt::Rect& elementRect,
    const winrt::ScrollingSnapPointsMode& snapPointsMode,
    double horizontalAlignmentRatio,
    double verticalAlignmentRatio,
    double horizontalOffset,
    double verticalOffset,
    _Out_ double* targetZoomedHorizontalOffset,
    _Out_ double* targetZoomedVerticalOffset)
{
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, L"%s[0x%p](H/V AlignmentRatio:%lf,%lf, H/V Offset:%f,%f, ElementRect:%s, Element:0x%p)\n",
        METH_NAME, this,
        horizontalAlignmentRatio, verticalAlignmentRatio,
        horizontalOffset, verticalOffset,
        TypeLogging::RectToString(elementRect).c_str(), element);

    ComputeBringIntoViewTargetOffsets(
        content,
        element,
        elementRect,
        snapPointsMode,
        horizontalAlignmentRatio,
        verticalAlignmentRatio,
        horizontalOffset,
        verticalOffset,
        targetZoomedHorizontalOffset,
        targetZoomedVerticalOffset,
        nullptr /*appliedOffsetX*/,
        nullptr /*appliedOffsetY*/,
        nullptr /*targetRect*/);
}

void ScrollPresenter::ComputeBringIntoViewTargetOffsets(
    const winrt::UIElement& content,
    const winrt::UIElement& element,
    const winrt::Rect& elementRect,
    const winrt::ScrollingSnapPointsMode& snapPointsMode,
    double horizontalAlignmentRatio,
    double verticalAlignmentRatio,
    double horizontalOffset,
    double verticalOffset,
    _Out_ double* targetZoomedHorizontalOffset,
    _Out_ double* targetZoomedVerticalOffset,
    _Out_opt_ double* appliedOffsetX,
    _Out_opt_ double* appliedOffsetY,
    _Out_opt_ winrt::Rect* targetRect)
{
    MUX_ASSERT(content);
    MUX_ASSERT(element);

    *targetZoomedHorizontalOffset = 0.0;
    *targetZoomedVerticalOffset = 0.0;

    if (appliedOffsetX)
    {
        *appliedOffsetX = 0.0;
    }

    if (appliedOffsetY)
    {
        *appliedOffsetY = 0.0;
    }

    if (targetRect)
    {
        *targetRect = {};
    }

    const winrt::Rect transformedRect = GetDescendantBounds(content, element, elementRect);

    double targetX = transformedRect.X;
    double targetWidth = transformedRect.Width;
    double targetY = transformedRect.Y;
    double targetHeight = transformedRect.Height;

    if (!isnan(horizontalAlignmentRatio))
    {
        // Account for the horizontal alignment ratio
        MUX_ASSERT(horizontalAlignmentRatio >= 0.0 && horizontalAlignmentRatio <= 1.0);

        targetX += (targetWidth - m_viewportWidth / m_zoomFactor) * horizontalAlignmentRatio;
        targetWidth = m_viewportWidth / m_zoomFactor;
    }

    if (!isnan(verticalAlignmentRatio))
    {
        // Account for the vertical alignment ratio
        MUX_ASSERT(verticalAlignmentRatio >= 0.0 && verticalAlignmentRatio <= 1.0);

        targetY += (targetHeight - m_viewportHeight / m_zoomFactor) * verticalAlignmentRatio;
        targetHeight = m_viewportHeight / m_zoomFactor;
    }

    double targetZoomedHorizontalOffsetTmp = ComputeZoomedOffsetWithMinimalChange(
        m_zoomedHorizontalOffset,
        m_zoomedHorizontalOffset + m_viewportWidth,
        targetX * m_zoomFactor,
        (targetX + targetWidth) * m_zoomFactor);
    double targetZoomedVerticalOffsetTmp = ComputeZoomedOffsetWithMinimalChange(
        m_zoomedVerticalOffset,
        m_zoomedVerticalOffset + m_viewportHeight,
        targetY * m_zoomFactor,
        (targetY + targetHeight) * m_zoomFactor);

    const double scrollableWidth = ScrollableWidth();
    const double scrollableHeight = ScrollableHeight();

    targetZoomedHorizontalOffsetTmp = std::clamp(targetZoomedHorizontalOffsetTmp, 0.0, scrollableWidth);
    targetZoomedVerticalOffsetTmp = std::clamp(targetZoomedVerticalOffsetTmp, 0.0, scrollableHeight);

    const double offsetX = horizontalOffset;
    const double offsetY = verticalOffset;
    double appliedOffsetXTmp = 0.0;
    double appliedOffsetYTmp = 0.0;

    // If the target offset is within bounds and an offset was provided, apply as much of it as possible while remaining within bounds.
    if (offsetX != 0.0 && targetZoomedHorizontalOffsetTmp >= 0.0)
    {
        if (targetZoomedHorizontalOffsetTmp <= scrollableWidth)
        {
            if (offsetX > 0.0)
            {
                appliedOffsetXTmp = std::min(targetZoomedHorizontalOffsetTmp, offsetX);
            }
            else
            {
                appliedOffsetXTmp = -std::min(scrollableWidth - targetZoomedHorizontalOffsetTmp, -offsetX);
            }
            targetZoomedHorizontalOffsetTmp -= appliedOffsetXTmp;
        }
    }

    if (offsetY != 0.0 && targetZoomedVerticalOffsetTmp >= 0.0)
    {
        if (targetZoomedVerticalOffsetTmp <= scrollableHeight)
        {
            if (offsetY > 0.0)
            {
                appliedOffsetYTmp = std::min(targetZoomedVerticalOffsetTmp, offsetY);
            }
            else
            {
                appliedOffsetYTmp = -std::min(scrollableHeight - targetZoomedVerticalOffsetTmp, -offsetY);
            }
            targetZoomedVerticalOffsetTmp -= appliedOffsetYTmp;
        }
    }

    MUX_ASSERT(targetZoomedHorizontalOffsetTmp >= 0.0);
    MUX_ASSERT(targetZoomedVerticalOffsetTmp >= 0.0);
    MUX_ASSERT(targetZoomedHorizontalOffsetTmp <= scrollableWidth);
    MUX_ASSERT(targetZoomedVerticalOffsetTmp <= scrollableHeight);

    if (snapPointsMode == winrt::ScrollingSnapPointsMode::Default)
    {
        // Finally adjust the target offsets based on snap points
        targetZoomedHorizontalOffsetTmp = ComputeValueAfterSnapPoints<winrt::ScrollSnapPointBase>(
            targetZoomedHorizontalOffsetTmp, m_sortedConsolidatedHorizontalSnapPoints);
        targetZoomedVerticalOffsetTmp = ComputeValueAfterSnapPoints<winrt::ScrollSnapPointBase>(
            targetZoomedVerticalOffsetTmp, m_sortedConsolidatedVerticalSnapPoints);

        // Make sure the target offsets are within the scrollable boundaries
        targetZoomedHorizontalOffsetTmp = std::clamp(targetZoomedHorizontalOffsetTmp, 0.0, scrollableWidth);
        targetZoomedVerticalOffsetTmp = std::clamp(targetZoomedVerticalOffsetTmp, 0.0, scrollableHeight);

        MUX_ASSERT(targetZoomedHorizontalOffsetTmp >= 0.0);
        MUX_ASSERT(targetZoomedVerticalOffsetTmp >= 0.0);
        MUX_ASSERT(targetZoomedHorizontalOffsetTmp <= scrollableWidth);
        MUX_ASSERT(targetZoomedVerticalOffsetTmp <= scrollableHeight);
    }

    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"targetZoomedHorizontalOffset", targetZoomedHorizontalOffsetTmp);
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"targetZoomedVerticalOffset", targetZoomedVerticalOffsetTmp);

    *targetZoomedHorizontalOffset = targetZoomedHorizontalOffsetTmp;
    *targetZoomedVerticalOffset = targetZoomedVerticalOffsetTmp;

    if (appliedOffsetX)
    {
        *appliedOffsetX = appliedOffsetXTmp;
    }

    if (appliedOffsetY)
    {
        *appliedOffsetY = appliedOffsetYTmp;
    }

    if (targetRect)
    {
        *targetRect = {
            static_cast<float>(targetX),
            static_cast<float>(targetY),
            static_cast<float>(targetWidth),
            static_cast<float>(targetHeight)
        };
    }
}

void ScrollPresenter::EnsureExpressionAnimationSources()
{
    if (!m_expressionAnimationSources)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();

        m_expressionAnimationSources = compositor.CreatePropertySet();
        m_expressionAnimationSources.InsertVector2(s_extentSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertVector2(s_viewportSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertVector2(s_offsetSourcePropertyName, { m_contentLayoutOffsetX, m_contentLayoutOffsetY });
        m_expressionAnimationSources.InsertVector2(s_positionSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertVector2(s_minPositionSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertVector2(s_maxPositionSourcePropertyName, { 0.0f, 0.0f });
        m_expressionAnimationSources.InsertScalar(s_zoomFactorSourcePropertyName, 0.0f);

        MUX_ASSERT(m_interactionTracker);
        MUX_ASSERT(!m_positionSourceExpressionAnimation);
        MUX_ASSERT(!m_minPositionSourceExpressionAnimation);
        MUX_ASSERT(!m_maxPositionSourceExpressionAnimation);
        MUX_ASSERT(!m_zoomFactorSourceExpressionAnimation);

        m_positionSourceExpressionAnimation = compositor.CreateExpressionAnimation(L"Vector2(it.Position.X, it.Position.Y)");
        m_positionSourceExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

        m_minPositionSourceExpressionAnimation = compositor.CreateExpressionAnimation(L"Vector2(it.MinPosition.X, it.MinPosition.Y)");
        m_minPositionSourceExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

        m_maxPositionSourceExpressionAnimation = compositor.CreateExpressionAnimation(L"Vector2(it.MaxPosition.X, it.MaxPosition.Y)");
        m_maxPositionSourceExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

        m_zoomFactorSourceExpressionAnimation = compositor.CreateExpressionAnimation(L"it.Scale");
        m_zoomFactorSourceExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

        StartExpressionAnimationSourcesAnimations();
        UpdateExpressionAnimationSources();
    }
}

void ScrollPresenter::EnsureInteractionTracker()
{
    if (!m_interactionTracker)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        MUX_ASSERT(!m_interactionTrackerOwner);
        m_interactionTrackerOwner = winrt::make_self<InteractionTrackerOwner>(*this).try_as<winrt::IInteractionTrackerOwner>();

        const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();
        m_interactionTracker = winrt::InteractionTracker::CreateWithOwner(compositor, m_interactionTrackerOwner);
    }
}

void ScrollPresenter::EnsureScrollPresenterVisualInteractionSource()
{
    if (!m_scrollPresenterVisualInteractionSource)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        EnsureInteractionTracker();

        const winrt::Visual scrollPresenterVisual = winrt::ElementCompositionPreview::GetElementVisual(*this);
        winrt::VisualInteractionSource scrollPresenterVisualInteractionSource = winrt::VisualInteractionSource::Create(scrollPresenterVisual);
        m_interactionTracker.InteractionSources().Add(scrollPresenterVisualInteractionSource);
        m_scrollPresenterVisualInteractionSource = scrollPresenterVisualInteractionSource;
        UpdateManipulationRedirectionMode();
        RaiseInteractionSourcesChanged();
    }
}

void ScrollPresenter::EnsureScrollControllerVisualInteractionSource(
    const winrt::Visual& panningElementAncestorVisual,
    ScrollPresenterDimension dimension)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR_INT, METH_NAME, this, panningElementAncestorVisual, dimension);

    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);
    MUX_ASSERT(m_interactionTracker);

    winrt::VisualInteractionSource scrollControllerVisualInteractionSource = winrt::VisualInteractionSource::Create(panningElementAncestorVisual);
    scrollControllerVisualInteractionSource.ManipulationRedirectionMode(winrt::VisualInteractionSourceRedirectionMode::CapableTouchpadOnly);
    scrollControllerVisualInteractionSource.PositionXChainingMode(winrt::InteractionChainingMode::Never);
    scrollControllerVisualInteractionSource.PositionYChainingMode(winrt::InteractionChainingMode::Never);
    scrollControllerVisualInteractionSource.ScaleChainingMode(winrt::InteractionChainingMode::Never);
    scrollControllerVisualInteractionSource.ScaleSourceMode(winrt::InteractionSourceMode::Disabled);
    m_interactionTracker.InteractionSources().Add(scrollControllerVisualInteractionSource);

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        MUX_ASSERT(m_horizontalScrollController);
        MUX_ASSERT(m_horizontalScrollControllerPanningInfo);
        MUX_ASSERT(!m_horizontalScrollControllerVisualInteractionSource);
        m_horizontalScrollControllerVisualInteractionSource = scrollControllerVisualInteractionSource;

        HookHorizontalScrollControllerInteractionSourceEvents(m_horizontalScrollControllerPanningInfo.get());
    }
    else
    {
        MUX_ASSERT(m_verticalScrollController);
        MUX_ASSERT(m_verticalScrollControllerPanningInfo);
        MUX_ASSERT(!m_verticalScrollControllerVisualInteractionSource);
        m_verticalScrollControllerVisualInteractionSource = scrollControllerVisualInteractionSource;

        HookVerticalScrollControllerInteractionSourceEvents(m_verticalScrollControllerPanningInfo.get());
    }

    RaiseInteractionSourcesChanged();
}

void ScrollPresenter::EnsureScrollControllerExpressionAnimationSources(
    ScrollPresenterDimension dimension)
{
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);
    MUX_ASSERT(m_interactionTracker);

    const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();
    winrt::CompositionPropertySet scrollControllerExpressionAnimationSources = nullptr;

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        if (m_horizontalScrollControllerExpressionAnimationSources)
        {
            return;
        }

        m_horizontalScrollControllerExpressionAnimationSources = scrollControllerExpressionAnimationSources = compositor.CreatePropertySet();
    }
    else
    {
        if (m_verticalScrollControllerExpressionAnimationSources)
        {
            return;
        }

        m_verticalScrollControllerExpressionAnimationSources = scrollControllerExpressionAnimationSources = compositor.CreatePropertySet();
    }

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, dimension);

    scrollControllerExpressionAnimationSources.InsertScalar(s_minOffsetPropertyName, 0.0f);
    scrollControllerExpressionAnimationSources.InsertScalar(s_maxOffsetPropertyName, 0.0f);
    scrollControllerExpressionAnimationSources.InsertScalar(s_offsetPropertyName, 0.0f);
    scrollControllerExpressionAnimationSources.InsertScalar(s_multiplierPropertyName, 1.0f);

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        MUX_ASSERT(!m_horizontalScrollControllerOffsetExpressionAnimation);
        MUX_ASSERT(!m_horizontalScrollControllerMaxOffsetExpressionAnimation);

        m_horizontalScrollControllerOffsetExpressionAnimation = compositor.CreateExpressionAnimation(L"it.Position.X - it.MinPosition.X");
        m_horizontalScrollControllerOffsetExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_horizontalScrollControllerMaxOffsetExpressionAnimation = compositor.CreateExpressionAnimation(L"it.MaxPosition.X - it.MinPosition.X");
        m_horizontalScrollControllerMaxOffsetExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
    }
    else
    {
        MUX_ASSERT(!m_verticalScrollControllerOffsetExpressionAnimation);
        MUX_ASSERT(!m_verticalScrollControllerMaxOffsetExpressionAnimation);

        m_verticalScrollControllerOffsetExpressionAnimation = compositor.CreateExpressionAnimation(L"it.Position.Y - it.MinPosition.Y");
        m_verticalScrollControllerOffsetExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_verticalScrollControllerMaxOffsetExpressionAnimation = compositor.CreateExpressionAnimation(L"it.MaxPosition.Y - it.MinPosition.Y");
        m_verticalScrollControllerMaxOffsetExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
    }
}

void ScrollPresenter::EnsurePositionBoundariesExpressionAnimations()
{
    if (!m_minPositionExpressionAnimation || !m_maxPositionExpressionAnimation)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();

        if (!m_minPositionExpressionAnimation)
        {
            m_minPositionExpressionAnimation = compositor.CreateExpressionAnimation();
        }
        if (!m_maxPositionExpressionAnimation)
        {
            m_maxPositionExpressionAnimation = compositor.CreateExpressionAnimation();
        }
    }
}

void ScrollPresenter::EnsureTransformExpressionAnimations()
{
    if (!m_translationExpressionAnimation || !m_zoomFactorExpressionAnimation)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();

        if (!m_translationExpressionAnimation)
        {
            m_translationExpressionAnimation = compositor.CreateExpressionAnimation();
        }

        if (!m_zoomFactorExpressionAnimation)
        {
            m_zoomFactorExpressionAnimation = compositor.CreateExpressionAnimation();
        }
    }
}

template <typename T>
void ScrollPresenter::SetupSnapPoints(
    std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
    ScrollPresenterDimension dimension)
{
    MUX_ASSERT(snapPointsSet);

    if (!m_interactionTracker)
    {
        EnsureInteractionTracker();
    }

    if (m_state == winrt::ScrollingInteractionState::Idle)
    {
        const double ignoredValue = [this, dimension]()
        {
            switch (dimension)
            {
            case ScrollPresenterDimension::VerticalScroll:
                return m_zoomedVerticalOffset / m_zoomFactor;
            case ScrollPresenterDimension::HorizontalScroll:
                return m_zoomedHorizontalOffset / m_zoomFactor;
            case ScrollPresenterDimension::ZoomFactor:
                return static_cast<double>(m_zoomFactor);
            default:
                MUX_ASSERT(false);
                return 0.0;
            }
        }();

        // When snap points are changed while in the Idle State, update
        // ignored snapping values for any potential start of an impulse inertia.
        UpdateSnapPointsIgnoredValue(snapPointsSet, ignoredValue);
    }

    // Update the regular and impulse actual applicable ranges.
    UpdateSnapPointsRanges(snapPointsSet, false /*forImpulseOnly*/);

    winrt::Compositor compositor = m_interactionTracker.Compositor();
    winrt::IVector<winrt::InteractionTrackerInertiaModifier> modifiers = winrt::make<Vector<winrt::InteractionTrackerInertiaModifier>>();

    winrt::hstring target = L"";
    winrt::hstring scale = L"";

    switch (dimension)
    {
    case ScrollPresenterDimension::HorizontalZoomFactor:
    case ScrollPresenterDimension::VerticalZoomFactor:
    case ScrollPresenterDimension::Scroll:
        //these ScrollPresenterDimensions are not expected
        MUX_ASSERT(false);
        break;
    case ScrollPresenterDimension::HorizontalScroll:
        target = s_naturalRestingPositionXPropertyName;
        scale = s_targetScalePropertyName;
        break;
    case ScrollPresenterDimension::VerticalScroll:
        target = s_naturalRestingPositionYPropertyName;
        scale = s_targetScalePropertyName;
        break;
    case ScrollPresenterDimension::ZoomFactor:
        target = s_naturalRestingScalePropertyName;
        scale = L"1.0";
        break;
    default:
        MUX_ASSERT(false);
    }

    // For older versions of windows the interaction tracker cannot accept empty collections of inertia modifiers
    if (snapPointsSet->size() == 0)
    {
        winrt::InteractionTrackerInertiaRestingValue modifier = winrt::InteractionTrackerInertiaRestingValue::Create(compositor);
        winrt::ExpressionAnimation conditionExpressionAnimation = compositor.CreateExpressionAnimation(L"false");
        winrt::ExpressionAnimation restingPointExpressionAnimation = compositor.CreateExpressionAnimation(L"this.Target." + target);

        modifier.Condition(conditionExpressionAnimation);
        modifier.RestingValue(restingPointExpressionAnimation);

        modifiers.Append(modifier);
    }
    else
    {
        for (auto snapPointWrapper : *snapPointsSet)
        {
            winrt::InteractionTrackerInertiaRestingValue modifier = GetInertiaRestingValue(
                snapPointWrapper,
                compositor,
                target,
                scale);

            modifiers.Append(modifier);
        }
    }

    switch (dimension)
    {
    case ScrollPresenterDimension::HorizontalZoomFactor:
    case ScrollPresenterDimension::VerticalZoomFactor:
    case ScrollPresenterDimension::Scroll:
        //these ScrollPresenterDimensions are not expected
        MUX_ASSERT(false);
        break;
    case ScrollPresenterDimension::HorizontalScroll:
        m_interactionTracker.ConfigurePositionXInertiaModifiers(modifiers);
        break;
    case ScrollPresenterDimension::VerticalScroll:
        m_interactionTracker.ConfigurePositionYInertiaModifiers(modifiers);
        break;
    case ScrollPresenterDimension::ZoomFactor:
        m_interactionTracker.ConfigureScaleInertiaModifiers(modifiers);
        break;
    default:
        MUX_ASSERT(false);
    }
}

//Snap points which have ApplicableRangeType = Optional are optional snap points, and their ActualApplicableRange should never be expanded beyond their ApplicableRange
//and will only shrink to accommodate other snap points which are positioned such that the midpoint between them is within the specified ApplicableRange.
//Snap points which have ApplicableRangeType = Mandatory are mandatory snap points and their ActualApplicableRange will expand or shrink to ensure that there is no
//space between it and its neighbors. If the neighbors are also mandatory, this point will be the midpoint between them. If the neighbors are optional then this
//point will fall on the midpoint or on the Optional neighbor's edge of ApplicableRange, whichever is furthest.
template <typename T>
void ScrollPresenter::UpdateSnapPointsRanges(
    std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
    bool forImpulseOnly)
{
    MUX_ASSERT(snapPointsSet);

    std::shared_ptr<SnapPointWrapper<T>> currentSnapPointWrapper = nullptr;
    std::shared_ptr<SnapPointWrapper<T>> previousSnapPointWrapper = nullptr;
    std::shared_ptr<SnapPointWrapper<T>> nextSnapPointWrapper = nullptr;

    for (auto snapPointWrapper : *snapPointsSet)
    {
        previousSnapPointWrapper = currentSnapPointWrapper;
        currentSnapPointWrapper = nextSnapPointWrapper;
        nextSnapPointWrapper = snapPointWrapper;

        if (currentSnapPointWrapper)
        {
            currentSnapPointWrapper->DetermineActualApplicableZone(
                previousSnapPointWrapper ? previousSnapPointWrapper.get() : nullptr,
                nextSnapPointWrapper ? nextSnapPointWrapper.get() : nullptr,
                forImpulseOnly);
        }
    }

    if (nextSnapPointWrapper)
    {
        nextSnapPointWrapper->DetermineActualApplicableZone(
            currentSnapPointWrapper ? currentSnapPointWrapper.get() : nullptr,
            nullptr,
            forImpulseOnly);
    }
}

template <typename T>
void ScrollPresenter::UpdateSnapPointsIgnoredValue(
    std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
    ScrollPresenterDimension dimension)
{
    const double newIgnoredValue = [this, dimension]()
    {
        switch (dimension)
        {
        case ScrollPresenterDimension::VerticalScroll:
            return m_zoomedVerticalOffset / m_zoomFactor;
        case ScrollPresenterDimension::HorizontalScroll:
            return m_zoomedHorizontalOffset / m_zoomFactor;
        case ScrollPresenterDimension::ZoomFactor:
            return static_cast<double>(m_zoomFactor);
        default:
            MUX_ASSERT(false);
            return 0.0;
        }
    }();

    if (UpdateSnapPointsIgnoredValue(snapPointsSet, newIgnoredValue))
    {
        // The ignored snap point value has changed.
        UpdateSnapPointsRanges(snapPointsSet, true /*forImpulseOnly*/);

        winrt::Compositor compositor = m_interactionTracker.Compositor();
        winrt::IVector<winrt::InteractionTrackerInertiaModifier> modifiers = winrt::make<Vector<winrt::InteractionTrackerInertiaModifier>>();

        for (auto snapPointWrapper : *snapPointsSet)
        {
            auto modifier = winrt::InteractionTrackerInertiaRestingValue::Create(compositor);
            auto const [conditionExpressionAnimation, restingValueExpressionAnimation] = snapPointWrapper->GetUpdatedExpressionAnimationsForImpulse();

            modifier.Condition(conditionExpressionAnimation);
            modifier.RestingValue(restingValueExpressionAnimation);

            modifiers.Append(modifier);
        }

        switch (dimension)
        {
        case ScrollPresenterDimension::VerticalScroll:
            m_interactionTracker.ConfigurePositionYInertiaModifiers(modifiers);
            break;
        case ScrollPresenterDimension::HorizontalScroll:
            m_interactionTracker.ConfigurePositionXInertiaModifiers(modifiers);
            break;
        case ScrollPresenterDimension::ZoomFactor:
            m_interactionTracker.ConfigureScaleInertiaModifiers(modifiers);
            break;
        }
    }
}

// Updates the ignored snapping value of the provided snap points set when inertia is caused by an impulse.
// Returns True when an old ignored value was reset or a new ignored value was set.
template <typename T>
bool ScrollPresenter::UpdateSnapPointsIgnoredValue(
    std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
    double newIgnoredValue)
{
    bool ignoredValueUpdated = false;

    for (auto snapPointWrapper : *snapPointsSet)
    {
        if (snapPointWrapper->ResetIgnoredValue())
        {
            ignoredValueUpdated = true;
            break;
        }
    }

    int snapCount = 0;

    for (auto snapPointWrapper : *snapPointsSet)
    {
        const SnapPointBase* snapPoint = SnapPointWrapper<T>::GetSnapPointFromWrapper(snapPointWrapper);

        snapCount += snapPoint->SnapCount();

        if (snapCount > 1)
        {
            break;
        }
    }

    if (snapCount > 1)
    {
        for (auto snapPointWrapper : *snapPointsSet)
        {
            if (snapPointWrapper->SnapsAt(newIgnoredValue))
            {
                snapPointWrapper->SetIgnoredValue(newIgnoredValue);
                ignoredValueUpdated = true;
                break;
            }
        }
    }

    return ignoredValueUpdated;
}

void ScrollPresenter::SetupInteractionTrackerBoundaries()
{
    if (!m_interactionTracker)
    {
        EnsureInteractionTracker();
        SetupInteractionTrackerZoomFactorBoundaries(
            MinZoomFactor(),
            MaxZoomFactor());
    }

    const winrt::UIElement content = Content();

    if (content && (!m_minPositionExpressionAnimation || !m_maxPositionExpressionAnimation))
    {
        EnsurePositionBoundariesExpressionAnimations();
        SetupPositionBoundariesExpressionAnimations(content);
    }
}

void ScrollPresenter::SetupInteractionTrackerZoomFactorBoundaries(
    double minZoomFactor, double maxZoomFactor)
{
    MUX_ASSERT(m_interactionTracker);

#ifdef DBG
    const float oldMinZoomFactorDbg = m_interactionTracker.MinScale();
#endif //DBG
    const float oldMaxZoomFactor = m_interactionTracker.MaxScale();

    minZoomFactor = std::max(0.0, minZoomFactor);
    maxZoomFactor = std::max(minZoomFactor, maxZoomFactor);

    const float newMinZoomFactor = static_cast<float>(minZoomFactor);
    const float newMaxZoomFactor = static_cast<float>(maxZoomFactor);

    if (newMinZoomFactor > oldMaxZoomFactor)
    {
        m_interactionTracker.MaxScale(newMaxZoomFactor);
        m_interactionTracker.MinScale(newMinZoomFactor);
    }
    else
    {
        m_interactionTracker.MinScale(newMinZoomFactor);
        m_interactionTracker.MaxScale(newMaxZoomFactor);
    }
}

// Configures the VisualInteractionSource instance associated with ScrollPresenter's Visual.
void ScrollPresenter::SetupScrollPresenterVisualInteractionSource()
{
    MUX_ASSERT(m_scrollPresenterVisualInteractionSource);

    SetupVisualInteractionSourceRailingMode(
        m_scrollPresenterVisualInteractionSource,
        ScrollPresenterDimension::HorizontalScroll,
        HorizontalScrollRailMode());

    SetupVisualInteractionSourceRailingMode(
        m_scrollPresenterVisualInteractionSource,
        ScrollPresenterDimension::VerticalScroll,
        VerticalScrollRailMode());

    SetupVisualInteractionSourceChainingMode(
        m_scrollPresenterVisualInteractionSource,
        ScrollPresenterDimension::HorizontalScroll,
        HorizontalScrollChainMode());

    SetupVisualInteractionSourceChainingMode(
        m_scrollPresenterVisualInteractionSource,
        ScrollPresenterDimension::VerticalScroll,
        VerticalScrollChainMode());

    SetupVisualInteractionSourceChainingMode(
        m_scrollPresenterVisualInteractionSource,
        ScrollPresenterDimension::ZoomFactor,
        ZoomChainMode());

    UpdateVisualInteractionSourceMode(
        ScrollPresenterDimension::HorizontalScroll);

    UpdateVisualInteractionSourceMode(
        ScrollPresenterDimension::VerticalScroll);

    SetupVisualInteractionSourceMode(
        m_scrollPresenterVisualInteractionSource,
        ZoomMode());

#ifdef IsMouseWheelZoomDisabled
    SetupVisualInteractionSourcePointerWheelConfig(
        m_scrollPresenterVisualInteractionSource,
        GetMouseWheelZoomMode());
#endif
}

// Configures the VisualInteractionSource instance associated with the Visual handed in
// through IScrollControllerPanningInfo::PanningElementAncestor.
void ScrollPresenter::SetupScrollControllerVisualInterationSource(
    ScrollPresenterDimension dimension)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);

    winrt::VisualInteractionSource scrollControllerVisualInteractionSource = nullptr;
    winrt::Visual panningElementAncestorVisual = nullptr;

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        scrollControllerVisualInteractionSource = m_horizontalScrollControllerVisualInteractionSource;
        if (m_horizontalScrollControllerPanningInfo)
        {
            const winrt::UIElement panningElementAncestor = m_horizontalScrollControllerPanningInfo.get().PanningElementAncestor();

            if (panningElementAncestor)
            {
                panningElementAncestorVisual = winrt::ElementCompositionPreview::GetElementVisual(panningElementAncestor);
            }
        }
    }
    else
    {
        scrollControllerVisualInteractionSource = m_verticalScrollControllerVisualInteractionSource;
        if (m_verticalScrollControllerPanningInfo)
        {
            const winrt::UIElement panningElementAncestor = m_verticalScrollControllerPanningInfo.get().PanningElementAncestor();

            if (panningElementAncestor)
            {
                panningElementAncestorVisual = winrt::ElementCompositionPreview::GetElementVisual(panningElementAncestor);
            }
        }
    }

    if (!panningElementAncestorVisual && scrollControllerVisualInteractionSource)
    {
        // The IScrollController no longer uses a Visual.
        winrt::VisualInteractionSource otherScrollControllerVisualInteractionSource =
            dimension == ScrollPresenterDimension::HorizontalScroll ? m_verticalScrollControllerVisualInteractionSource : m_horizontalScrollControllerVisualInteractionSource;

        if (otherScrollControllerVisualInteractionSource != scrollControllerVisualInteractionSource)
        {
            // The horizontal and vertical IScrollController implementations are not using the same Visual,
            // so the old VisualInteractionSource can be discarded.
            m_interactionTracker.InteractionSources().Remove(scrollControllerVisualInteractionSource);
            StopScrollControllerExpressionAnimationSourcesAnimations(dimension);
            if (dimension == ScrollPresenterDimension::HorizontalScroll)
            {
                m_horizontalScrollControllerVisualInteractionSource = nullptr;
                m_horizontalScrollControllerExpressionAnimationSources = nullptr;
                m_horizontalScrollControllerOffsetExpressionAnimation = nullptr;
                m_horizontalScrollControllerMaxOffsetExpressionAnimation = nullptr;
            }
            else
            {
                m_verticalScrollControllerVisualInteractionSource = nullptr;
                m_verticalScrollControllerExpressionAnimationSources = nullptr;
                m_verticalScrollControllerOffsetExpressionAnimation = nullptr;
                m_verticalScrollControllerMaxOffsetExpressionAnimation = nullptr;
            }

            RaiseInteractionSourcesChanged();
        }
        else
        {
            // The horizontal and vertical IScrollController implementations were using the same Visual,
            // so the old VisualInteractionSource cannot be discarded.
            if (dimension == ScrollPresenterDimension::HorizontalScroll)
            {
                scrollControllerVisualInteractionSource.PositionXSourceMode(winrt::InteractionSourceMode::Disabled);
                scrollControllerVisualInteractionSource.IsPositionXRailsEnabled(false);
            }
            else
            {
                scrollControllerVisualInteractionSource.PositionYSourceMode(winrt::InteractionSourceMode::Disabled);
                scrollControllerVisualInteractionSource.IsPositionYRailsEnabled(false);
            }
        }
        return;
    }
    else if (panningElementAncestorVisual)
    {
        if (!scrollControllerVisualInteractionSource)
        {
            // The IScrollController now uses a Visual.
            winrt::VisualInteractionSource otherScrollControllerVisualInteractionSource =
                dimension == ScrollPresenterDimension::HorizontalScroll ? m_verticalScrollControllerVisualInteractionSource : m_horizontalScrollControllerVisualInteractionSource;

            if (!otherScrollControllerVisualInteractionSource || otherScrollControllerVisualInteractionSource.Source() != panningElementAncestorVisual)
            {
                // That Visual is not shared with the other dimension, so create a new VisualInteractionSource for it.
                EnsureScrollControllerVisualInteractionSource(panningElementAncestorVisual, dimension);
            }
            else
            {
                // That Visual is shared with the other dimension, so share the existing VisualInteractionSource as well.
                if (dimension == ScrollPresenterDimension::HorizontalScroll)
                {
                    m_horizontalScrollControllerVisualInteractionSource = otherScrollControllerVisualInteractionSource;
                }
                else
                {
                    m_verticalScrollControllerVisualInteractionSource = otherScrollControllerVisualInteractionSource;
                }
            }
            EnsureScrollControllerExpressionAnimationSources(dimension);
            StartScrollControllerExpressionAnimationSourcesAnimations(dimension);
        }

        winrt::Orientation orientation;
        bool isRailEnabled;

        // Setup the VisualInteractionSource instance.
        if (dimension == ScrollPresenterDimension::HorizontalScroll)
        {
            orientation = m_horizontalScrollControllerPanningInfo.get().PanOrientation();
            isRailEnabled = m_horizontalScrollControllerPanningInfo.get().IsRailEnabled();

            if (orientation == winrt::Orientation::Horizontal)
            {
                m_horizontalScrollControllerVisualInteractionSource.PositionXSourceMode(winrt::InteractionSourceMode::EnabledWithoutInertia);
                m_horizontalScrollControllerVisualInteractionSource.IsPositionXRailsEnabled(isRailEnabled);
            }
            else
            {
                m_horizontalScrollControllerVisualInteractionSource.PositionYSourceMode(winrt::InteractionSourceMode::EnabledWithoutInertia);
                m_horizontalScrollControllerVisualInteractionSource.IsPositionYRailsEnabled(isRailEnabled);
            }
        }
        else
        {
            orientation = m_verticalScrollControllerPanningInfo.get().PanOrientation();
            isRailEnabled = m_verticalScrollControllerPanningInfo.get().IsRailEnabled();

            if (orientation == winrt::Orientation::Horizontal)
            {
                m_verticalScrollControllerVisualInteractionSource.PositionXSourceMode(winrt::InteractionSourceMode::EnabledWithoutInertia);
                m_verticalScrollControllerVisualInteractionSource.IsPositionXRailsEnabled(isRailEnabled);
            }
            else
            {
                m_verticalScrollControllerVisualInteractionSource.PositionYSourceMode(winrt::InteractionSourceMode::EnabledWithoutInertia);
                m_verticalScrollControllerVisualInteractionSource.IsPositionYRailsEnabled(isRailEnabled);
            }
        }

        if (!scrollControllerVisualInteractionSource)
        {
            SetupScrollControllerVisualInterationSourcePositionModifiers(
                dimension,
                orientation);
        }
    }
}

// Configures the Position input modifiers of the VisualInteractionSource associated
// with an IScrollController Visual.  The scalar called Multiplier from the CompositionPropertySet
// used in IScrollControllerPanningInfo::SetPanningElementExpressionAnimationSources determines the relative
// speed of IScrollControllerPanningInfo panning element compared to the ScrollPresenter.Content element.
// The panning element is clamped based on the Interaction's MinPosition and MaxPosition values.
// Four CompositionConditionalValue instances cover all scenarios:
//  - the Position is moved closer to InteractionTracker.MinPosition while the multiplier is negative.
//  - the Position is moved closer to InteractionTracker.MinPosition while the multiplier is positive.
//  - the Position is moved closer to InteractionTracker.MaxPosition while the multiplier is negative.
//  - the Position is moved closer to InteractionTracker.MaxPosition while the multiplier is positive.
void ScrollPresenter::SetupScrollControllerVisualInterationSourcePositionModifiers(
    ScrollPresenterDimension dimension,            // Direction of the ScrollPresenter.Content Visual movement.
    const winrt::Orientation& orientation)  // Direction of the IScrollController's Visual movement.
{
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);
    MUX_ASSERT(m_interactionTracker);

    winrt::VisualInteractionSource scrollControllerVisualInteractionSource = dimension == ScrollPresenterDimension::HorizontalScroll ?
        m_horizontalScrollControllerVisualInteractionSource : m_verticalScrollControllerVisualInteractionSource;
    winrt::CompositionPropertySet scrollControllerExpressionAnimationSources = dimension == ScrollPresenterDimension::HorizontalScroll ?
        m_horizontalScrollControllerExpressionAnimationSources : m_verticalScrollControllerExpressionAnimationSources;

    MUX_ASSERT(scrollControllerVisualInteractionSource);
    MUX_ASSERT(scrollControllerExpressionAnimationSources);

    winrt::Compositor compositor = scrollControllerVisualInteractionSource.Compositor();
    const winrt::CompositionConditionalValue ccvs[4]{ winrt::CompositionConditionalValue::Create(compositor), winrt::CompositionConditionalValue::Create(compositor), winrt::CompositionConditionalValue::Create(compositor), winrt::CompositionConditionalValue::Create(compositor) };
    const winrt::ExpressionAnimation conditions[4]{ compositor.CreateExpressionAnimation(), compositor.CreateExpressionAnimation(), compositor.CreateExpressionAnimation(), compositor.CreateExpressionAnimation() };
    const winrt::ExpressionAnimation values[4]{ compositor.CreateExpressionAnimation(), compositor.CreateExpressionAnimation(), compositor.CreateExpressionAnimation(), compositor.CreateExpressionAnimation() };
    for (int index = 0; index < 4; index++)
    {
        ccvs[index].Condition(conditions[index]);
        ccvs[index].Value(values[index]);

        values[index].SetReferenceParameter(L"sceas", scrollControllerExpressionAnimationSources);
        values[index].SetReferenceParameter(L"scvis", scrollControllerVisualInteractionSource);
        values[index].SetReferenceParameter(L"it", m_interactionTracker);
    }

    for (int index = 0; index < 3; index++)
    {
        conditions[index].SetReferenceParameter(L"scvis", scrollControllerVisualInteractionSource);
        conditions[index].SetReferenceParameter(L"sceas", scrollControllerExpressionAnimationSources);
    }
    conditions[3].Expression(L"true");

    const auto modifiersVector = winrt::single_threaded_vector<winrt::CompositionConditionalValue>();

    for (int index = 0; index < 4; index++)
    {
        modifiersVector.Append(ccvs[index]);
    }

    if (orientation == winrt::Orientation::Horizontal)
    {
        conditions[0].Expression(L"scvis.DeltaPosition.X < 0.0f && sceas.Multiplier < 0.0f");
        conditions[1].Expression(L"scvis.DeltaPosition.X < 0.0f && sceas.Multiplier >= 0.0f");
        conditions[2].Expression(L"scvis.DeltaPosition.X >= 0.0f && sceas.Multiplier < 0.0f");
        // Case #4 <==> scvis.DeltaPosition.X >= 0.0f && sceas.Multiplier > 0.0f, uses conditions[3].Expression(L"true").
        if (dimension == ScrollPresenterDimension::HorizontalScroll)
        {
            const auto expressionClampToMinPosition = L"min(sceas.Multiplier * scvis.DeltaPosition.X, it.Position.X - it.MinPosition.X)";
            const auto expressionClampToMaxPosition = L"max(sceas.Multiplier * scvis.DeltaPosition.X, it.Position.X - it.MaxPosition.X)";

            values[0].Expression(expressionClampToMinPosition);
            values[1].Expression(expressionClampToMaxPosition);
            values[2].Expression(expressionClampToMaxPosition);
            values[3].Expression(expressionClampToMinPosition);
            scrollControllerVisualInteractionSource.ConfigureDeltaPositionXModifiers(modifiersVector);
        }
        else
        {
            const auto expressionClampToMinPosition = L"min(sceas.Multiplier * scvis.DeltaPosition.X, it.Position.Y - it.MinPosition.Y)";
            const auto expressionClampToMaxPosition = L"max(sceas.Multiplier * scvis.DeltaPosition.X, it.Position.Y - it.MaxPosition.Y)";

            values[0].Expression(expressionClampToMinPosition);
            values[1].Expression(expressionClampToMaxPosition);
            values[2].Expression(expressionClampToMaxPosition);
            values[3].Expression(expressionClampToMinPosition);
            scrollControllerVisualInteractionSource.ConfigureDeltaPositionYModifiers(modifiersVector);

            // When the IScrollController's Visual moves horizontally and controls the vertical ScrollPresenter.Content movement, make sure that the
            // vertical finger movements do not affect the ScrollPresenter.Content vertically. The vertical component of the finger movement is filtered out.
            winrt::CompositionConditionalValue ccvOrtho = winrt::CompositionConditionalValue::Create(compositor);
            winrt::ExpressionAnimation conditionOrtho = compositor.CreateExpressionAnimation(L"true");
            winrt::ExpressionAnimation valueOrtho = compositor.CreateExpressionAnimation(L"0");
            ccvOrtho.Condition(conditionOrtho);
            ccvOrtho.Value(valueOrtho);

            auto modifiersVectorOrtho = winrt::single_threaded_vector<winrt::CompositionConditionalValue>();
            modifiersVectorOrtho.Append(ccvOrtho);

            scrollControllerVisualInteractionSource.ConfigureDeltaPositionXModifiers(modifiersVectorOrtho);
        }
    }
    else
    {
        conditions[0].Expression(L"scvis.DeltaPosition.Y < 0.0f && sceas.Multiplier < 0.0f");
        conditions[1].Expression(L"scvis.DeltaPosition.Y < 0.0f && sceas.Multiplier >= 0.0f");
        conditions[2].Expression(L"scvis.DeltaPosition.Y >= 0.0f && sceas.Multiplier < 0.0f");
        // Case #4 <==> scvis.DeltaPosition.Y >= 0.0f && sceas.Multiplier > 0.0f, uses conditions[3].Expression(L"true").
        if (dimension == ScrollPresenterDimension::HorizontalScroll)
        {
            const auto expressionClampToMinPosition = L"min(sceas.Multiplier * scvis.DeltaPosition.Y, it.Position.X - it.MinPosition.X)";
            const auto expressionClampToMaxPosition = L"max(sceas.Multiplier * scvis.DeltaPosition.Y, it.Position.X - it.MaxPosition.X)";

            values[0].Expression(expressionClampToMinPosition);
            values[1].Expression(expressionClampToMaxPosition);
            values[2].Expression(expressionClampToMaxPosition);
            values[3].Expression(expressionClampToMinPosition);
            scrollControllerVisualInteractionSource.ConfigureDeltaPositionXModifiers(modifiersVector);

            // When the IScrollController's Visual moves vertically and controls the horizontal ScrollPresenter.Content movement, make sure that the
            // horizontal finger movements do not affect the ScrollPresenter.Content horizontally. The horizontal component of the finger movement is filtered out.
            winrt::CompositionConditionalValue ccvOrtho = winrt::CompositionConditionalValue::Create(compositor);
            winrt::ExpressionAnimation conditionOrtho = compositor.CreateExpressionAnimation(L"true");
            winrt::ExpressionAnimation valueOrtho = compositor.CreateExpressionAnimation(L"0");
            ccvOrtho.Condition(conditionOrtho);
            ccvOrtho.Value(valueOrtho);

            auto modifiersVectorOrtho = winrt::single_threaded_vector<winrt::CompositionConditionalValue>();
            modifiersVectorOrtho.Append(ccvOrtho);

            scrollControllerVisualInteractionSource.ConfigureDeltaPositionYModifiers(modifiersVectorOrtho);
        }
        else
        {
            const auto expressionClampToMinPosition = L"min(sceas.Multiplier * scvis.DeltaPosition.Y, it.Position.Y - it.MinPosition.Y)";
            const auto expressionClampToMaxPosition = L"max(sceas.Multiplier * scvis.DeltaPosition.Y, it.Position.Y - it.MaxPosition.Y)";

            values[0].Expression(expressionClampToMinPosition);
            values[1].Expression(expressionClampToMaxPosition);
            values[2].Expression(expressionClampToMaxPosition);
            values[3].Expression(expressionClampToMinPosition);
            scrollControllerVisualInteractionSource.ConfigureDeltaPositionYModifiers(modifiersVector);
        }
    }
}

void ScrollPresenter::SetupVisualInteractionSourceRailingMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollPresenterDimension dimension,
    const winrt::ScrollingRailMode& railingMode)
{
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        visualInteractionSource.IsPositionXRailsEnabled(railingMode == winrt::ScrollingRailMode::Enabled);
    }
    else
    {
        visualInteractionSource.IsPositionYRailsEnabled(railingMode == winrt::ScrollingRailMode::Enabled);
    }
}

void ScrollPresenter::SetupVisualInteractionSourceChainingMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollPresenterDimension dimension,
    const winrt::ScrollingChainMode& chainingMode)
{
    MUX_ASSERT(visualInteractionSource);

    const winrt::InteractionChainingMode interactionChainingMode = InteractionChainingModeFromChainingMode(chainingMode);

    switch (dimension)
    {
    case ScrollPresenterDimension::HorizontalScroll:
        visualInteractionSource.PositionXChainingMode(interactionChainingMode);
        break;
    case ScrollPresenterDimension::VerticalScroll:
        visualInteractionSource.PositionYChainingMode(interactionChainingMode);
        break;
    case ScrollPresenterDimension::ZoomFactor:
        visualInteractionSource.ScaleChainingMode(interactionChainingMode);
        break;
    default:
        MUX_ASSERT(false);
    }
}

void ScrollPresenter::SetupVisualInteractionSourceMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollPresenterDimension dimension,
    const winrt::ScrollingScrollMode& scrollMode)
{
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(scrollMode == winrt::ScrollingScrollMode::Enabled || scrollMode == winrt::ScrollingScrollMode::Disabled);

    const winrt::InteractionSourceMode interactionSourceMode = InteractionSourceModeFromScrollMode(scrollMode);

    switch (dimension)
    {
    case ScrollPresenterDimension::HorizontalScroll:
        visualInteractionSource.PositionXSourceMode(interactionSourceMode);
        break;
    case ScrollPresenterDimension::VerticalScroll:
        visualInteractionSource.PositionYSourceMode(interactionSourceMode);
        break;
    default:
        MUX_ASSERT(false);
    }
}

void ScrollPresenter::SetupVisualInteractionSourceMode(
    const winrt::VisualInteractionSource& visualInteractionSource,
    const winrt::ScrollingZoomMode& zoomMode)
{
    MUX_ASSERT(visualInteractionSource);

    visualInteractionSource.ScaleSourceMode(InteractionSourceModeFromZoomMode(zoomMode));
}

#ifdef IsMouseWheelScrollDisabled
void ScrollPresenter::SetupVisualInteractionSourcePointerWheelConfig(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollPresenterDimension dimension,
    const winrt::ScrollingScrollMode& scrollMode)
{
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(scrollMode == winrt::ScrollingScrollMode::Enabled || scrollMode == winrt::ScrollingScrollMode::Disabled);

    winrt::InteractionSourceRedirectionMode interactionSourceRedirectionMode = InteractionSourceRedirectionModeFromScrollMode(scrollMode);

    switch (dimension)
    {
    case ScrollPresenterDimension::HorizontalScroll:
        visualInteractionSource.PointerWheelConfig().PositionXSourceMode(interactionSourceRedirectionMode);
        break;
    case ScrollPresenterDimension::VerticalScroll:
        visualInteractionSource.PointerWheelConfig().PositionYSourceMode(interactionSourceRedirectionMode);
        break;
    default:
        MUX_ASSERT(false);
    }
}
#endif

#ifdef IsMouseWheelZoomDisabled
void ScrollPresenter::SetupVisualInteractionSourcePointerWheelConfig(
    const winrt::VisualInteractionSource& visualInteractionSource,
    const winrt::ScrollingZoomMode& zoomMode)
{
    MUX_ASSERT(visualInteractionSource);

    visualInteractionSource.PointerWheelConfig().ScaleSourceMode(InteractionSourceRedirectionModeFromZoomMode(zoomMode));
}
#endif

void ScrollPresenter::SetupVisualInteractionSourceRedirectionMode(
    const winrt::VisualInteractionSource& visualInteractionSource)
{
    MUX_ASSERT(visualInteractionSource);

    winrt::VisualInteractionSourceRedirectionMode redirectionMode = winrt::VisualInteractionSourceRedirectionMode::CapableTouchpadOnly;

    if (!IsInputKindIgnored(winrt::ScrollingInputKinds::MouseWheel))
    {
        redirectionMode = winrt::VisualInteractionSourceRedirectionMode::CapableTouchpadAndPointerWheel;
    }

    visualInteractionSource.ManipulationRedirectionMode(redirectionMode);
}

void ScrollPresenter::SetupVisualInteractionSourceCenterPointModifier(
    const winrt::VisualInteractionSource& visualInteractionSource,
    ScrollPresenterDimension dimension,
    bool flowDirectionChanged)
{
    MUX_ASSERT(visualInteractionSource);
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);
    MUX_ASSERT(m_interactionTracker);

    const bool isHorizontalDimension = dimension == ScrollPresenterDimension::HorizontalScroll;
    const bool isRightToLeftDirection = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const float xamlLayoutOffset = isHorizontalDimension ? m_contentLayoutOffsetX : m_contentLayoutOffsetY;

    // Note that resetting to 'nullptr' when xamlLayoutOffset is 0 and isRightToLeftDirection is False is not working, so the branch below
    // is used instead when flowDirectionChanged is True.
    if (xamlLayoutOffset == 0.0f && !(isHorizontalDimension && (isRightToLeftDirection || flowDirectionChanged)))
    {
        if (isHorizontalDimension)
        {
            visualInteractionSource.ConfigureCenterPointXModifiers(nullptr);
            m_interactionTracker.ConfigureCenterPointXInertiaModifiers(nullptr);
        }
        else
        {
            visualInteractionSource.ConfigureCenterPointYModifiers(nullptr);
            m_interactionTracker.ConfigureCenterPointYInertiaModifiers(nullptr);
        }
    }
    else
    {
        winrt::Compositor compositor = visualInteractionSource.Compositor();
        winrt::ExpressionAnimation conditionCenterPointModifier = compositor.CreateExpressionAnimation(L"true");
        winrt::CompositionConditionalValue conditionValueCenterPointModifier = winrt::CompositionConditionalValue::Create(compositor);
        winrt::hstring valueCenterPointModifierExpression;

        if (isHorizontalDimension)
        {
            valueCenterPointModifierExpression = isRightToLeftDirection ? L"-visualInteractionSource.CenterPoint.X + xamlLayoutOffset" : L"visualInteractionSource.CenterPoint.X - xamlLayoutOffset";
        }
        else
        {
            valueCenterPointModifierExpression = L"visualInteractionSource.CenterPoint.Y - xamlLayoutOffset";
        }

        winrt::ExpressionAnimation valueCenterPointModifier = compositor.CreateExpressionAnimation(valueCenterPointModifierExpression);

        valueCenterPointModifier.SetReferenceParameter(L"visualInteractionSource", visualInteractionSource);
        valueCenterPointModifier.SetScalarParameter(L"xamlLayoutOffset", xamlLayoutOffset);

        conditionValueCenterPointModifier.Condition(conditionCenterPointModifier);
        conditionValueCenterPointModifier.Value(valueCenterPointModifier);

        auto centerPointModifiers = winrt::single_threaded_vector<winrt::CompositionConditionalValue>();
        centerPointModifiers.Append(conditionValueCenterPointModifier);

        if (isHorizontalDimension)
        {
            visualInteractionSource.ConfigureCenterPointXModifiers(centerPointModifiers);
            m_interactionTracker.ConfigureCenterPointXInertiaModifiers(centerPointModifiers);
        }
        else
        {
            visualInteractionSource.ConfigureCenterPointYModifiers(centerPointModifiers);
            m_interactionTracker.ConfigureCenterPointYInertiaModifiers(centerPointModifiers);
        }
    }
}

winrt::ScrollingScrollMode ScrollPresenter::GetComputedScrollMode(ScrollPresenterDimension dimension, bool ignoreZoomMode)
{
    winrt::ScrollingScrollMode oldComputedScrollMode;
    winrt::ScrollingScrollMode newComputedScrollMode;

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        oldComputedScrollMode = ComputedHorizontalScrollMode();
        newComputedScrollMode = HorizontalScrollMode();
    }
    else
    {
        MUX_ASSERT(dimension == ScrollPresenterDimension::VerticalScroll);
        oldComputedScrollMode = ComputedVerticalScrollMode();
        newComputedScrollMode = VerticalScrollMode();
    }

    if (newComputedScrollMode == winrt::ScrollingScrollMode::Auto)
    {
        if (!ignoreZoomMode && ZoomMode() == winrt::ScrollingZoomMode::Enabled)
        {
            // Allow scrolling when zooming is turned on so that the Content does not get stuck in the given dimension
            // when it becomes smaller than the viewport.
            newComputedScrollMode = winrt::ScrollingScrollMode::Enabled;
        }
        else
        {
            if (dimension == ScrollPresenterDimension::HorizontalScroll)
            {
                // Enable horizontal scrolling only when the Content's width is larger than the ScrollPresenter's width
                newComputedScrollMode = ScrollableWidth() > 0.0 ? winrt::ScrollingScrollMode::Enabled : winrt::ScrollingScrollMode::Disabled;
            }
            else
            {
                // Enable vertical scrolling only when the Content's height is larger than the ScrollPresenter's height
                newComputedScrollMode = ScrollableHeight() > 0.0 ? winrt::ScrollingScrollMode::Enabled : winrt::ScrollingScrollMode::Disabled;
            }
        }
    }

    if (oldComputedScrollMode != newComputedScrollMode)
    {
        if (dimension == ScrollPresenterDimension::HorizontalScroll)
        {
            SetValue(s_ComputedHorizontalScrollModeProperty, box_value(newComputedScrollMode));
        }
        else
        {
            SetValue(s_ComputedVerticalScrollModeProperty, box_value(newComputedScrollMode));
        }
    }

    return newComputedScrollMode;
}

#ifdef IsMouseWheelScrollDisabled
winrt::ScrollingScrollMode ScrollPresenter::GetComputedMouseWheelScrollMode(ScrollPresenterDimension dimension)
{
    // TODO: c.f. Task 18569498 - Consider public IsMouseWheelHorizontalScrollDisabled/IsMouseWheelVerticalScrollDisabled properties
    return GetComputedScrollMode(dimension);
}
#endif

#ifdef IsMouseWheelZoomDisabled
winrt::ScrollingZoomMode ScrollPresenter::GetMouseWheelZoomMode()
{
    // TODO: c.f. Task 18569498 - Consider public IsMouseWheelZoomDisabled properties
    return ZoomMode();
}
#endif

double ScrollPresenter::GetLayoutRoundFactor() const
{
    if (UseLayoutRounding())
    {
        if (const auto xamlRoot = XamlRoot())
        {
            return xamlRoot.RasterizationScale();
        }
    }

    return 0.0;
}

double ScrollPresenter::GetComputedMaxWidth(
    double defaultMaxWidth,
    const winrt::FrameworkElement& content) const
{
    MUX_ASSERT(content);

    const winrt::Thickness contentMargin = content.Margin();
    const double marginWidth = contentMargin.Left + contentMargin.Right;
    double computedMaxWidth = defaultMaxWidth;
    double width = content.Width();
    double minWidth = content.MinWidth();
    double maxWidth = content.MaxWidth();

    if (!isnan(width))
    {
        width = std::max(0.0, width + marginWidth);
        computedMaxWidth = width;
    }
    if (!isnan(minWidth))
    {
        minWidth = std::max(0.0, minWidth + marginWidth);
        computedMaxWidth = std::max(computedMaxWidth, minWidth);
    }
    if (!isnan(maxWidth))
    {
        maxWidth = std::max(0.0, maxWidth + marginWidth);
        computedMaxWidth = std::min(computedMaxWidth, maxWidth);
    }

    return computedMaxWidth;
}

double ScrollPresenter::GetComputedMaxHeight(
    double defaultMaxHeight,
    const winrt::FrameworkElement& content) const
{
    MUX_ASSERT(content);

    const winrt::Thickness contentMargin = content.Margin();
    const double marginHeight = contentMargin.Top + contentMargin.Bottom;
    double computedMaxHeight = defaultMaxHeight;
    double height = content.Height();
    double minHeight = content.MinHeight();
    double maxHeight = content.MaxHeight();

    if (!isnan(height))
    {
        height = std::max(0.0, height + marginHeight);
        computedMaxHeight = height;
    }
    if (!isnan(minHeight))
    {
        minHeight = std::max(0.0, minHeight + marginHeight);
        computedMaxHeight = std::max(computedMaxHeight, minHeight);
    }
    if (!isnan(maxHeight))
    {
        maxHeight = std::max(0.0, maxHeight + marginHeight);
        computedMaxHeight = std::min(computedMaxHeight, maxHeight);
    }

    return computedMaxHeight;
}

// Computes the content's layout offsets at zoomFactor 1 coming from the Margin property and the difference between the extent and render sizes.
winrt::float2 ScrollPresenter::GetArrangeRenderSizesDelta(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"m_unzoomedExtentWidth", m_unzoomedExtentWidth);
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"m_unzoomedExtentHeight", m_unzoomedExtentHeight);
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"content.RenderSize().Width", content.RenderSize().Width);
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"content.RenderSize().Height", content.RenderSize().Height);

    double deltaX = m_unzoomedExtentWidth - content.RenderSize().Width;
    double deltaY = m_unzoomedExtentHeight - content.RenderSize().Height;

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (contentAsFE)
    {
        const winrt::HorizontalAlignment horizontalAlignment = contentAsFE.HorizontalAlignment();
        const winrt::VerticalAlignment verticalAlignment = contentAsFE.VerticalAlignment();
        const winrt::Thickness contentMargin = contentAsFE.Margin();

        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"horizontalAlignment", horizontalAlignment);
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"verticalAlignment", verticalAlignment);
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"contentMargin.Left", contentMargin.Left);
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"contentMargin.Right", contentMargin.Right);
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"contentMargin.Top", contentMargin.Top);
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"contentMargin.Bottom", contentMargin.Bottom);

        if (horizontalAlignment == winrt::HorizontalAlignment::Left)
        {
            deltaX = 0.0f;
        }
        else
        {
            deltaX -= contentMargin.Left + contentMargin.Right;
        }

        if (verticalAlignment == winrt::VerticalAlignment::Top)
        {
            deltaY = 0.0f;
        }
        else
        {
            deltaY -= contentMargin.Top + contentMargin.Bottom;
        }

        if (horizontalAlignment == winrt::HorizontalAlignment::Center ||
            horizontalAlignment == winrt::HorizontalAlignment::Stretch)
        {
            deltaX /= 2.0f;
        }

        if (verticalAlignment == winrt::VerticalAlignment::Center ||
            verticalAlignment == winrt::VerticalAlignment::Stretch)
        {
            deltaY /= 2.0f;
        }

        deltaX += contentMargin.Left;
        deltaY += contentMargin.Top;
    }

    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"deltaX", deltaX);
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"deltaY", deltaY);

    return winrt::float2{ static_cast<float>(deltaX), static_cast<float>(deltaY) };
}

// Returns the expression for the m_minPositionExpressionAnimation animation based on the Content.HorizontalAlignment,
// Content.VerticalAlignment, InteractionTracker.Scale, Content arrange size (which takes Content.Margin into account) and
// ScrollPresenterVisual.Size properties.
winrt::hstring ScrollPresenter::GetMinPositionExpression(
    const winrt::UIElement& content) const
{
    return StringUtil::FormatString(L"Vector3(%1!s!, %2!s!, 0.0f)", GetMinPositionXExpression(content).c_str(), GetMinPositionYExpression(content).c_str());
}

winrt::hstring ScrollPresenter::GetMinPositionXExpression(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        if (contentAsFE)
        {
            const std::wstring_view maxOffset{ L"(contentSizeX * it.Scale - scrollPresenterVisual.Size.X)" };

            if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
                contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
            {
                return StringUtil::FormatString(L"%1!s! >= 0 ? -%1!s! - contentLayoutOffsetX : -%1!s! / 2.0f - contentLayoutOffsetX", maxOffset.data());
            }
            else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
            {
                return StringUtil::FormatString(L"-%1!s! - contentLayoutOffsetX", maxOffset.data());
            }
        }

        return winrt::hstring(L"-Max(0.0f, contentSizeX * it.Scale - scrollPresenterVisual.Size.X) - contentLayoutOffsetX");
    }

    if (contentAsFE)
    {
        const std::wstring_view maxOffset{ L"contentSizeX * it.Scale - scrollPresenterVisual.Size.X" };

        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            return StringUtil::FormatString(L"Min(0.0f, (%1!s!) / 2.0f) + contentLayoutOffsetX", maxOffset.data());
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            return StringUtil::FormatString(L"Min(0.0f, %1!s!) + contentLayoutOffsetX", maxOffset.data());
        }
    }

    return winrt::hstring(L"contentLayoutOffsetX");
}

winrt::hstring ScrollPresenter::GetMinPositionYExpression(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (contentAsFE)
    {
        const std::wstring_view maxOffset{ L"contentSizeY * it.Scale - scrollPresenterVisual.Size.Y" };

        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            return StringUtil::FormatString(L"Min(0.0f, (%1!s!) / 2.0f) + contentLayoutOffsetY", maxOffset.data());
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            return StringUtil::FormatString(L"Min(0.0f, %1!s!) + contentLayoutOffsetY", maxOffset.data());
        }
    }

    return winrt::hstring(L"contentLayoutOffsetY");
}

// Returns the expression for the m_maxPositionExpressionAnimation animation based on the Content.HorizontalAlignment,
// Content.VerticalAlignment, InteractionTracker.Scale, Content arrange size (which takes Content.Margin into account) and
// ScrollPresenterVisual.Size properties.
winrt::hstring ScrollPresenter::GetMaxPositionExpression(
    const winrt::UIElement& content) const
{
    return StringUtil::FormatString(L"Vector3(%1!s!, %2!s!, 0.0f)", GetMaxPositionXExpression(content).c_str(), GetMaxPositionYExpression(content).c_str());
}

winrt::hstring ScrollPresenter::GetMaxPositionXExpression(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        if (contentAsFE)
        {
            const std::wstring_view maxOffset{ L"contentSizeX * it.Scale - scrollPresenterVisual.Size.X" };

            if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
                contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
            {
                return StringUtil::FormatString(L"-Min(0.0f, (%1!s!) / 2.0f) - contentLayoutOffsetX", maxOffset.data());
            }
            else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
            {
                return StringUtil::FormatString(L"-Min(0.0f, %1!s!) - contentLayoutOffsetX", maxOffset.data());
            }
        }

        return winrt::hstring(L"-contentLayoutOffsetX");
    }

    if (contentAsFE)
    {
        const std::wstring_view maxOffset{ L"(contentSizeX * it.Scale - scrollPresenterVisual.Size.X)" };

        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            return StringUtil::FormatString(L"%1!s! >= 0 ? %1!s! + contentLayoutOffsetX : %1!s! / 2.0f + contentLayoutOffsetX", maxOffset.data());
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            return StringUtil::FormatString(L"%1!s! + contentLayoutOffsetX", maxOffset.data());
        }
    }

    return winrt::hstring(L"Max(0.0f, contentSizeX * it.Scale - scrollPresenterVisual.Size.X) + contentLayoutOffsetX");
}

winrt::hstring ScrollPresenter::GetMaxPositionYExpression(
    const winrt::UIElement& content) const
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

    if (contentAsFE)
    {
        const std::wstring_view maxOffset{ L"(contentSizeY * it.Scale - scrollPresenterVisual.Size.Y)" };

        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            return StringUtil::FormatString(L"%1!s! >= 0 ? %1!s! + contentLayoutOffsetY : %1!s! / 2.0f + contentLayoutOffsetY", maxOffset.data());
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            return StringUtil::FormatString(L"%1!s! + contentLayoutOffsetY", maxOffset.data());
        }
    }

    return winrt::hstring(L"Max(0.0f, contentSizeY * it.Scale - scrollPresenterVisual.Size.Y) + contentLayoutOffsetY");
}

winrt::CompositionAnimation ScrollPresenter::GetPositionAnimation(
    double zoomedHorizontalOffset,
    double zoomedVerticalOffset,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    int32_t offsetsChangeCorrelationId)
{
    MUX_ASSERT(m_interactionTracker);

    int64_t minDuration = s_offsetsChangeMinMs;
    int64_t maxDuration = s_offsetsChangeMaxMs;
    int64_t unitDuration = s_offsetsChangeMsPerUnit;
    const bool isHorizontalScrollControllerRequest = static_cast<char>(operationTrigger) & static_cast<char>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest);
    const bool isVerticalScrollControllerRequest = static_cast<char>(operationTrigger) & static_cast<char>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest);
    const int64_t distance = static_cast<int64_t>(sqrt(pow(zoomedHorizontalOffset - m_zoomedHorizontalOffset, 2.0) + pow(zoomedVerticalOffset - m_zoomedVerticalOffset, 2.0)));
    const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();
    winrt::Vector3KeyFrameAnimation positionAnimation = compositor.CreateVector3KeyFrameAnimation();
    com_ptr<ScrollPresenterTestHooks> globalTestHooks = ScrollPresenterTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        int unitDurationTestOverride;
        int minDurationTestOverride;
        int maxDurationTestOverride;

        globalTestHooks->GetOffsetsChangeVelocityParameters(unitDurationTestOverride, minDurationTestOverride, maxDurationTestOverride);

        minDuration = minDurationTestOverride;
        maxDuration = maxDurationTestOverride;
        unitDuration = unitDurationTestOverride;
    }

    const winrt::float2 endPosition = ComputePositionFromOffsets(zoomedHorizontalOffset, zoomedVerticalOffset);

    positionAnimation.InsertKeyFrame(1.0f, winrt::float3(endPosition, 0.0f));
    positionAnimation.Duration(winrt::TimeSpan::duration(std::clamp(distance * unitDuration, minDuration, maxDuration) * 10000));

    const winrt::float2 startPosition{ m_interactionTracker.Position().x, m_interactionTracker.Position().y };

    if (isHorizontalScrollControllerRequest || isVerticalScrollControllerRequest)
    {
        winrt::CompositionAnimation customAnimation = nullptr;

        if (isHorizontalScrollControllerRequest && m_horizontalScrollController)
        {
            customAnimation = m_horizontalScrollController.get().GetScrollAnimation(
                offsetsChangeCorrelationId,
                startPosition,
                endPosition,
                positionAnimation);
        }
        if (isVerticalScrollControllerRequest && m_verticalScrollController)
        {
            customAnimation = m_verticalScrollController.get().GetScrollAnimation(
                offsetsChangeCorrelationId,
                startPosition,
                endPosition,
                customAnimation ? customAnimation : positionAnimation);
        }
        return customAnimation ? customAnimation : positionAnimation;
    }

    return RaiseScrollAnimationStarting(positionAnimation, startPosition, endPosition, offsetsChangeCorrelationId);
}

winrt::CompositionAnimation ScrollPresenter::GetZoomFactorAnimation(
    float zoomFactor,
    const winrt::float2& centerPoint,
    int32_t zoomFactorChangeCorrelationId)
{
    int64_t minDuration = s_zoomFactorChangeMinMs;
    int64_t maxDuration = s_zoomFactorChangeMaxMs;
    int64_t unitDuration = s_zoomFactorChangeMsPerUnit;
    const int64_t distance = static_cast<int64_t>(abs(zoomFactor - m_zoomFactor));
    const winrt::Compositor compositor = winrt::ElementCompositionPreview::GetElementVisual(*this).Compositor();
    winrt::ScalarKeyFrameAnimation zoomFactorAnimation = compositor.CreateScalarKeyFrameAnimation();
    com_ptr<ScrollPresenterTestHooks> globalTestHooks = ScrollPresenterTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        int unitDurationTestOverride;
        int minDurationTestOverride;
        int maxDurationTestOverride;

        globalTestHooks->GetZoomFactorChangeVelocityParameters(unitDurationTestOverride, minDurationTestOverride, maxDurationTestOverride);

        minDuration = minDurationTestOverride;
        maxDuration = maxDurationTestOverride;
        unitDuration = unitDurationTestOverride;
    }

    zoomFactorAnimation.InsertKeyFrame(1.0f, zoomFactor);
    zoomFactorAnimation.Duration(winrt::TimeSpan::duration(std::clamp(distance * unitDuration, minDuration, maxDuration) * 10000));

    return RaiseZoomAnimationStarting(zoomFactorAnimation, zoomFactor, centerPoint, zoomFactorChangeCorrelationId);
}

int32_t ScrollPresenter::GetNextViewChangeCorrelationId()
{
    return (m_latestViewChangeCorrelationId == std::numeric_limits<int>::max()) ? 0 : m_latestViewChangeCorrelationId + 1;
}

void ScrollPresenter::SetupPositionBoundariesExpressionAnimations(
    const winrt::UIElement& content)
{
    MUX_ASSERT(content);
    MUX_ASSERT(m_minPositionExpressionAnimation);
    MUX_ASSERT(m_maxPositionExpressionAnimation);
    MUX_ASSERT(m_interactionTracker);

    const winrt::Visual scrollPresenterVisual = winrt::ElementCompositionPreview::GetElementVisual(*this);

    winrt::hstring s = m_minPositionExpressionAnimation.Expression();

    if (s.empty())
    {
        m_minPositionExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_minPositionExpressionAnimation.SetReferenceParameter(L"scrollPresenterVisual", scrollPresenterVisual);
    }

    m_minPositionExpressionAnimation.Expression(GetMinPositionExpression(content));

    s = m_maxPositionExpressionAnimation.Expression();

    if (s.empty())
    {
        m_maxPositionExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
        m_maxPositionExpressionAnimation.SetReferenceParameter(L"scrollPresenterVisual", scrollPresenterVisual);
    }

    m_maxPositionExpressionAnimation.Expression(GetMaxPositionExpression(content));

    UpdatePositionBoundaries(content);
}

void ScrollPresenter::SetupTransformExpressionAnimations(
    const winrt::UIElement& content)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(content);
    MUX_ASSERT(m_translationExpressionAnimation);
    MUX_ASSERT(m_zoomFactorExpressionAnimation);
    MUX_ASSERT(m_interactionTracker);

    const winrt::float2 arrangeRenderSizesDelta = GetArrangeRenderSizesDelta(content);
    const bool isRightToLeftDirection = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const bool isContentImage = content.try_as<winrt::Image>() != nullptr;
    winrt::hstring translationExpression;

    if (isRightToLeftDirection)
    {
        if (isContentImage)
        {
            m_translationExpressionAnimation.SetScalarParameter(L"contentSizeX", static_cast<float>(m_unzoomedExtentWidth));
            translationExpression = L"Vector3(it.Position.X + (it.Scale - 1.0f) * (adjustment.X + contentSizeX), -it.Position.Y + (it.Scale - 1.0f) * adjustment.Y, 0.0f)";
        }
        else
        {
            translationExpression = L"Vector3(it.Position.X + (it.Scale - 1.0f) * adjustment.X, -it.Position.Y + (it.Scale - 1.0f) * adjustment.Y, 0.0f)";
        }
    }
    else
    {
        translationExpression = L"Vector3(-it.Position.X + (it.Scale - 1.0f) * adjustment.X, -it.Position.Y + (it.Scale - 1.0f) * adjustment.Y, 0.0f)";
    }

    m_translationExpressionAnimation.Expression(translationExpression);

    m_translationExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);
    m_translationExpressionAnimation.SetVector2Parameter(L"adjustment", arrangeRenderSizesDelta);

    m_zoomFactorExpressionAnimation.Expression(L"Vector3(it.Scale, it.Scale, 1.0f)");
    m_zoomFactorExpressionAnimation.SetReferenceParameter(L"it", m_interactionTracker);

    StartTransformExpressionAnimations(content);
}

void ScrollPresenter::StartTransformExpressionAnimations(
    const winrt::UIElement& content)
{
    if (content)
    {
        auto const zoomFactorPropertyName = GetVisualTargetedPropertyName(ScrollPresenterDimension::ZoomFactor);
        auto const scrollPropertyName = GetVisualTargetedPropertyName(ScrollPresenterDimension::Scroll);

        m_translationExpressionAnimation.Target(scrollPropertyName);
        m_zoomFactorExpressionAnimation.Target(zoomFactorPropertyName);

        content.StartAnimation(m_translationExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, scrollPropertyName /*propertyName*/);

        content.StartAnimation(m_zoomFactorExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, zoomFactorPropertyName /*propertyName*/);
    }
}

void ScrollPresenter::StopTransformExpressionAnimations(
    const winrt::UIElement& content)
{
    if (content)
    {
        auto const scrollPropertyName = GetVisualTargetedPropertyName(ScrollPresenterDimension::Scroll);

        content.StopAnimation(m_translationExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(false /*isExpressionAnimationStarted*/, scrollPropertyName /*propertyName*/);

        auto const zoomFactorPropertyName = GetVisualTargetedPropertyName(ScrollPresenterDimension::ZoomFactor);

        content.StopAnimation(m_zoomFactorExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(false /*isExpressionAnimationStarted*/, zoomFactorPropertyName /*propertyName*/);
    }
}

// Returns True when ScrollPresenter::OnCompositionTargetRendering calls are not needed for restarting the Translation and Scale animations.
bool ScrollPresenter::StartTranslationAndZoomFactorExpressionAnimations(bool interruptCountdown)
{
    if (m_translationAndZoomFactorAnimationsRestartTicksCountdown > 0)
    {
        // A Translation and Scale animations restart is pending after the Idle State was reached or a zoom factor change operation completed.
        m_translationAndZoomFactorAnimationsRestartTicksCountdown--;

        if (m_translationAndZoomFactorAnimationsRestartTicksCountdown == 0 || interruptCountdown)
        {
            // Countdown is over or state is no longer Idle, restart the Translation and Scale animations.
            MUX_ASSERT(m_interactionTracker);

            SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, m_animationRestartZoomFactor, m_zoomFactor);

            if (m_translationAndZoomFactorAnimationsRestartTicksCountdown > 0)
            {
                MUX_ASSERT(interruptCountdown);

                SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, m_translationAndZoomFactorAnimationsRestartTicksCountdown);
                m_translationAndZoomFactorAnimationsRestartTicksCountdown = 0;
            }

            StartTransformExpressionAnimations(Content());
        }
        else
        {
            // Countdown needs to continue.
            return false;
        }
    }

    return true;
}

void ScrollPresenter::StopTranslationAndZoomFactorExpressionAnimations()
{
    if (m_zoomFactorExpressionAnimation && m_animationRestartZoomFactor != m_zoomFactor)
    {
        // The zoom factor has changed since the last restart of the Translation and Scale animations.
        const winrt::UIElement content = Content();

        if (m_translationAndZoomFactorAnimationsRestartTicksCountdown == 0)
        {
            SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, m_animationRestartZoomFactor, m_zoomFactor);

            // Stop Translation and Scale animations to trigger rasterization of Content, to avoid fuzzy text rendering for instance.
            StopTransformExpressionAnimations(content);

            // Trigger ScrollPresenter::OnCompositionTargetRendering calls in order to re-establish the Translation and Scale animations
            // after the Content rasterization was triggered within a few ticks.
            HookCompositionTargetRendering();
        }

        m_animationRestartZoomFactor = m_zoomFactor;
        m_translationAndZoomFactorAnimationsRestartTicksCountdown = s_translationAndZoomFactorAnimationsRestartTicks;
    }
}

void ScrollPresenter::StartExpressionAnimationSourcesAnimations()
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(m_expressionAnimationSources);
    MUX_ASSERT(m_positionSourceExpressionAnimation);
    MUX_ASSERT(m_minPositionSourceExpressionAnimation);
    MUX_ASSERT(m_maxPositionSourceExpressionAnimation);
    MUX_ASSERT(m_zoomFactorSourceExpressionAnimation);

    m_expressionAnimationSources.StartAnimation(s_positionSourcePropertyName, m_positionSourceExpressionAnimation);
    RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_positionSourcePropertyName /*propertyName*/);

    m_expressionAnimationSources.StartAnimation(s_minPositionSourcePropertyName, m_minPositionSourceExpressionAnimation);
    RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_minPositionSourcePropertyName /*propertyName*/);

    m_expressionAnimationSources.StartAnimation(s_maxPositionSourcePropertyName, m_maxPositionSourceExpressionAnimation);
    RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_maxPositionSourcePropertyName /*propertyName*/);

    m_expressionAnimationSources.StartAnimation(s_zoomFactorSourcePropertyName, m_zoomFactorSourceExpressionAnimation);
    RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_zoomFactorSourcePropertyName /*propertyName*/);
}

void ScrollPresenter::StartScrollControllerExpressionAnimationSourcesAnimations(
    ScrollPresenterDimension dimension)
{
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        MUX_ASSERT(m_horizontalScrollControllerExpressionAnimationSources);
        MUX_ASSERT(m_horizontalScrollControllerOffsetExpressionAnimation);
        MUX_ASSERT(m_horizontalScrollControllerMaxOffsetExpressionAnimation);

        m_horizontalScrollControllerExpressionAnimationSources.StartAnimation(s_offsetPropertyName, m_horizontalScrollControllerOffsetExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_offsetPropertyName /*propertyName*/);

        m_horizontalScrollControllerExpressionAnimationSources.StartAnimation(s_maxOffsetPropertyName, m_horizontalScrollControllerMaxOffsetExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_maxOffsetPropertyName /*propertyName*/);
    }
    else
    {
        MUX_ASSERT(m_verticalScrollControllerExpressionAnimationSources);
        MUX_ASSERT(m_verticalScrollControllerOffsetExpressionAnimation);
        MUX_ASSERT(m_verticalScrollControllerMaxOffsetExpressionAnimation);

        m_verticalScrollControllerExpressionAnimationSources.StartAnimation(s_offsetPropertyName, m_verticalScrollControllerOffsetExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_offsetPropertyName /*propertyName*/);

        m_verticalScrollControllerExpressionAnimationSources.StartAnimation(s_maxOffsetPropertyName, m_verticalScrollControllerMaxOffsetExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_maxOffsetPropertyName /*propertyName*/);
    }
}

void ScrollPresenter::StopScrollControllerExpressionAnimationSourcesAnimations(
    ScrollPresenterDimension dimension)
{
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        MUX_ASSERT(m_horizontalScrollControllerExpressionAnimationSources);

        m_horizontalScrollControllerExpressionAnimationSources.StopAnimation(s_offsetPropertyName);
        RaiseExpressionAnimationStatusChanged(false /*isExpressionAnimationStarted*/, s_offsetPropertyName /*propertyName*/);

        m_horizontalScrollControllerExpressionAnimationSources.StopAnimation(s_maxOffsetPropertyName);
        RaiseExpressionAnimationStatusChanged(false /*isExpressionAnimationStarted*/, s_maxOffsetPropertyName /*propertyName*/);
    }
    else
    {
        MUX_ASSERT(m_verticalScrollControllerExpressionAnimationSources);

        m_verticalScrollControllerExpressionAnimationSources.StopAnimation(s_offsetPropertyName);
        RaiseExpressionAnimationStatusChanged(false /*isExpressionAnimationStarted*/, s_offsetPropertyName /*propertyName*/);

        m_verticalScrollControllerExpressionAnimationSources.StopAnimation(s_maxOffsetPropertyName);
        RaiseExpressionAnimationStatusChanged(false /*isExpressionAnimationStarted*/, s_maxOffsetPropertyName /*propertyName*/);
    }
}

winrt::InteractionChainingMode ScrollPresenter::InteractionChainingModeFromChainingMode(
    const winrt::ScrollingChainMode& chainingMode)
{
    switch (chainingMode)
    {
    case winrt::ScrollingChainMode::Always:
        return winrt::InteractionChainingMode::Always;
    case winrt::ScrollingChainMode::Auto:
        return winrt::InteractionChainingMode::Auto;
    default:
        return winrt::InteractionChainingMode::Never;
    }
}

#ifdef IsMouseWheelScrollDisabled
winrt::InteractionSourceRedirectionMode ScrollPresenter::InteractionSourceRedirectionModeFromScrollMode(
    const winrt::ScrollingScrollMode& scrollMode)
{
    MUX_ASSERT(scrollMode == winrt::ScrollingScrollMode::Enabled || scrollMode == winrt::ScrollingScrollMode::Disabled);

    return scrollMode == winrt::ScrollingScrollMode::Enabled ? winrt::InteractionSourceRedirectionMode::Enabled : winrt::InteractionSourceRedirectionMode::Disabled;
}
#endif

#ifdef IsMouseWheelZoomDisabled
winrt::InteractionSourceRedirectionMode ScrollPresenter::InteractionSourceRedirectionModeFromZoomMode(
    const winrt::ScrollingZoomMode& zoomMode)
{
    return zoomMode == winrt::ScrollingZoomMode::Enabled ? winrt::InteractionSourceRedirectionMode::Enabled : winrt::InteractionSourceRedirectionMode::Disabled;
}
#endif

winrt::InteractionSourceMode ScrollPresenter::InteractionSourceModeFromScrollMode(
    const winrt::ScrollingScrollMode& scrollMode)
{
    return scrollMode == winrt::ScrollingScrollMode::Enabled ? winrt::InteractionSourceMode::EnabledWithInertia : winrt::InteractionSourceMode::Disabled;
}

winrt::InteractionSourceMode ScrollPresenter::InteractionSourceModeFromZoomMode(
    const winrt::ScrollingZoomMode& zoomMode)
{
    return zoomMode == winrt::ScrollingZoomMode::Enabled ? winrt::InteractionSourceMode::EnabledWithInertia : winrt::InteractionSourceMode::Disabled;
}

double ScrollPresenter::ComputeZoomedOffsetWithMinimalChange(
    double viewportStart,
    double viewportEnd,
    double childStart,
    double childEnd)
{
    const bool above = childStart < viewportStart && childEnd < viewportEnd;
    const bool below = childEnd > viewportEnd&& childStart > viewportStart;
    const bool larger = (childEnd - childStart) > (viewportEnd - viewportStart);

    // # CHILD POSITION   CHILD SIZE   SCROLL   REMEDY
    // 1 Above viewport   <= viewport  Down     Align top edge of content & viewport
    // 2 Above viewport   >  viewport  Down     Align bottom edge of content & viewport
    // 3 Below viewport   <= viewport  Up       Align bottom edge of content & viewport
    // 4 Below viewport   >  viewport  Up       Align top edge of content & viewport
    // 5 Entirely within viewport      NA       No change
    // 6 Spanning viewport             NA       No change
    if ((above && !larger) || (below && larger))
    {
        // Cases 1 & 4
        return childStart;
    }
    else if (above || below)
    {
        // Cases 2 & 3
        return childEnd - viewportEnd + viewportStart;
    }

    // cases 5 & 6
    return viewportStart;
}

winrt::Rect ScrollPresenter::GetDescendantBounds(
    const winrt::UIElement& content,
    const winrt::UIElement& descendant,
    const winrt::Rect& descendantRect)
{
    MUX_ASSERT(content);

    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();
    const winrt::GeneralTransform transform = descendant.TransformToVisual(content);
    winrt::Thickness contentMargin{};

    if (contentAsFE)
    {
        contentMargin = contentAsFE.Margin();
    }

    return transform.TransformBounds(winrt::Rect{
        static_cast<float>(contentMargin.Left + descendantRect.X),
        static_cast<float>(contentMargin.Top + descendantRect.Y),
        descendantRect.Width,
        descendantRect.Height });
}

winrt::ScrollingAnimationMode ScrollPresenter::GetComputedAnimationMode(
    winrt::ScrollingAnimationMode const& animationMode)
{
    if (animationMode == winrt::ScrollingAnimationMode::Auto)
    {
        const bool isAnimationsEnabled = []()
        {
            auto globalTestHooks = ScrollPresenterTestHooks::GetGlobalTestHooks();

            if (globalTestHooks && globalTestHooks->IsAnimationsEnabledOverride())
            {
                return globalTestHooks->IsAnimationsEnabledOverride().Value();
            }
            else
            {
                return SharedHelpers::IsAnimationsEnabled();
            }
        }();

        return isAnimationsEnabled ? winrt::ScrollingAnimationMode::Enabled : winrt::ScrollingAnimationMode::Disabled;
    }

    return animationMode;
}

bool ScrollPresenter::IsZoomFactorBoundaryValid(
    double value)
{
    return !isnan(value) && isfinite(value);
}

void ScrollPresenter::ValidateZoomFactoryBoundary(double value)
{
    if (!IsZoomFactorBoundaryValid(value))
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

// Returns the target property path, according to the availability of the ElementCompositionPreview::SetIsTranslationEnabled method,
// and the provided dimension.
wstring_view ScrollPresenter::GetVisualTargetedPropertyName(ScrollPresenterDimension dimension)
{
    switch (dimension)
    {
        case ScrollPresenterDimension::Scroll:
            return s_translationPropertyName;
        default:
            MUX_ASSERT(dimension == ScrollPresenterDimension::ZoomFactor);
            return s_scalePropertyName;
    }
}

// Invoked by both ScrollPresenter and ScrollViewer controls
bool ScrollPresenter::IsAnchorRatioValid(
    double value)
{
    return isnan(value) || (isfinite(value) && value >= 0.0 && value <= 1.0);
}

void ScrollPresenter::ValidateAnchorRatio(double value)
{
    if (!IsAnchorRatioValid(value))
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

bool ScrollPresenter::IsElementValidAnchor(
    const winrt::UIElement& element)
{
    return IsElementValidAnchor(element, Content());
}

// Invoked by ScrollPresenterTestHooks
void ScrollPresenter::SetContentLayoutOffsetXDbg(float contentLayoutOffsetX)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, contentLayoutOffsetX, m_contentLayoutOffsetX);

    if (m_contentLayoutOffsetX != contentLayoutOffsetX)
    {
        UpdateOffset(ScrollPresenterDimension::HorizontalScroll, m_zoomedHorizontalOffset - contentLayoutOffsetX + m_contentLayoutOffsetX);
        m_contentLayoutOffsetX = contentLayoutOffsetX;
        InvalidateArrange();
        OnContentLayoutOffsetChanged(ScrollPresenterDimension::HorizontalScroll);
        OnViewChanged(true /*horizontalOffsetChanged*/, false /*verticalOffsetChanged*/);
    }
}

void ScrollPresenter::SetContentLayoutOffsetYDbg(float contentLayoutOffsetY)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, contentLayoutOffsetY, m_contentLayoutOffsetY);

    if (m_contentLayoutOffsetY != contentLayoutOffsetY)
    {
        UpdateOffset(ScrollPresenterDimension::VerticalScroll, m_zoomedVerticalOffset - contentLayoutOffsetY + m_contentLayoutOffsetY);
        m_contentLayoutOffsetY = contentLayoutOffsetY;
        InvalidateArrange();
        OnContentLayoutOffsetChanged(ScrollPresenterDimension::VerticalScroll);
        OnViewChanged(false /*horizontalOffsetChanged*/, true /*verticalOffsetChanged*/);
    }
}

winrt::hstring ScrollPresenter::GetTransformExpressionAnimationExpressionDbg() const
{
    if (m_translationExpressionAnimation)
    {
        return m_translationExpressionAnimation.Expression();
    }

    return L"";
}

void ScrollPresenter::SetTransformExpressionAnimationExpressionDbg(winrt::hstring const& transformExpressionAnimationExpression)
{
    if (m_translationExpressionAnimation)
    {
        const winrt::UIElement content = Content();

        StopTransformExpressionAnimations(content);

        m_translationExpressionAnimation.Expression(transformExpressionAnimationExpression);

        StartTransformExpressionAnimations(content);
    }
}

winrt::hstring ScrollPresenter::GetMinPositionExpressionAnimationExpressionDbg() const
{
    if (m_minPositionExpressionAnimation)
    {
        return m_minPositionExpressionAnimation.Expression();
    }

    return L"";
}

void ScrollPresenter::SetMinPositionExpressionAnimationExpressionDbg(winrt::hstring const& minPositionExpressionAnimationExpression)
{
    if (m_minPositionExpressionAnimation)
    {
        m_minPositionExpressionAnimation.Expression(minPositionExpressionAnimationExpression);

        if (m_interactionTracker)
        {
            m_interactionTracker.StartAnimation(s_minPositionSourcePropertyName, m_minPositionExpressionAnimation);
        }
    }
}

winrt::hstring ScrollPresenter::GetMaxPositionExpressionAnimationExpressionDbg() const
{
    if (m_maxPositionExpressionAnimation)
    {
        return m_maxPositionExpressionAnimation.Expression();
    }

    return L"";
}

void ScrollPresenter::SetMaxPositionExpressionAnimationExpressionDbg(winrt::hstring const& maxPositionExpressionAnimationExpression)
{
    if (m_maxPositionExpressionAnimation)
    {
        m_maxPositionExpressionAnimation.Expression(maxPositionExpressionAnimationExpression);

        if (m_interactionTracker)
        {
            m_interactionTracker.StartAnimation(s_maxPositionSourcePropertyName, m_maxPositionExpressionAnimation);
        }
    }
}

winrt::float2 ScrollPresenter::GetArrangeRenderSizesDeltaDbg()
{
    winrt::float2 arrangeRenderSizesDelta{};
    const winrt::UIElement content = Content();

    if (content)
    {
        arrangeRenderSizesDelta = GetArrangeRenderSizesDelta(content);
    }

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, arrangeRenderSizesDelta.x, arrangeRenderSizesDelta.y);

    return arrangeRenderSizesDelta;
}

winrt::float2 ScrollPresenter::GetPositionDbg()
{
    if (m_interactionTracker)
    {
        const winrt::float2 position{ m_interactionTracker.Position().x, m_interactionTracker.Position().y };

        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, position.x, position.y);

        return position;
    }

    return {};
}

winrt::float2 ScrollPresenter::GetMinPositionDbg()
{
    winrt::float2 minPosition{};

    ComputeMinMaxPositions(m_zoomFactor, &minPosition, nullptr);

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, minPosition.x, minPosition.y);

    return minPosition;
}

void ScrollPresenter::SetMinPositionDbg(winrt::float2 minPosition)
{
#ifdef DBG
    m_minPositionOverrideDbg = minPosition;
#endif // DBG
}

winrt::float2 ScrollPresenter::GetMaxPositionDbg()
{
    winrt::float2 maxPosition{};

    ComputeMinMaxPositions(m_zoomFactor, nullptr, &maxPosition);

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, maxPosition.x, maxPosition.y);

    return maxPosition;
}

void ScrollPresenter::SetMaxPositionDbg(winrt::float2 maxPosition)
{
#ifdef DBG
    m_maxPositionOverrideDbg = maxPosition;
#endif // DBG
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollPresenter::GetConsolidatedScrollSnapPointsDbg(ScrollPresenterDimension dimension)
{
    winrt::IVector<winrt::ScrollSnapPointBase> snapPoints = winrt::make<Vector<winrt::ScrollSnapPointBase>>();
    std::set<std::shared_ptr<SnapPointWrapper<winrt::ScrollSnapPointBase>>, SnapPointWrapperComparator<winrt::ScrollSnapPointBase>> snapPointsSet;

    switch (dimension)
    {
    case ScrollPresenterDimension::VerticalScroll:
        snapPointsSet = m_sortedConsolidatedVerticalSnapPoints;
        break;
    case ScrollPresenterDimension::HorizontalScroll:
        snapPointsSet = m_sortedConsolidatedHorizontalSnapPoints;
        break;
    default:
        MUX_ASSERT(false);
    }

    for (std::shared_ptr<SnapPointWrapper<winrt::ScrollSnapPointBase>> snapPointWrapper : snapPointsSet)
    {
        snapPoints.Append(snapPointWrapper->SnapPoint());
    }
    return snapPoints;
}

winrt::IVector<winrt::ZoomSnapPointBase> ScrollPresenter::GetConsolidatedZoomSnapPointsDbg()
{
    winrt::IVector<winrt::ZoomSnapPointBase> snapPoints = winrt::make<Vector<winrt::ZoomSnapPointBase>>();

    for (std::shared_ptr<SnapPointWrapper<winrt::ZoomSnapPointBase>> snapPointWrapper : m_sortedConsolidatedZoomSnapPoints)
    {
        snapPoints.Append(snapPointWrapper->SnapPoint());
    }
    return snapPoints;
}

std::shared_ptr<SnapPointWrapper<winrt::ScrollSnapPointBase>> ScrollPresenter::GetScrollSnapPointWrapperDbg(ScrollPresenterDimension dimension, winrt::ScrollSnapPointBase const& scrollSnapPoint)
{
    const auto& snapPointsSet = [&]()
    {
        switch (dimension)
        {
        case ScrollPresenterDimension::VerticalScroll:
            return m_sortedConsolidatedVerticalSnapPoints;
            break;
        case ScrollPresenterDimension::HorizontalScroll:
            return m_sortedConsolidatedHorizontalSnapPoints;
            break;
        default:
            MUX_ASSERT(false);
            return m_sortedConsolidatedVerticalSnapPoints;
        }
    }();

    for (std::shared_ptr<SnapPointWrapper<winrt::ScrollSnapPointBase>> snapPointWrapper : snapPointsSet)
    {
        winrt::ScrollSnapPointBase winrtScrollSnapPoint = snapPointWrapper->SnapPoint().as<winrt::ScrollSnapPointBase>();

        if (winrtScrollSnapPoint == scrollSnapPoint)
        {
            return snapPointWrapper;
        }
    }

    return nullptr;
}

std::shared_ptr<SnapPointWrapper<winrt::ZoomSnapPointBase>> ScrollPresenter::GetZoomSnapPointWrapperDbg(winrt::ZoomSnapPointBase const& zoomSnapPoint)
{
    for (std::shared_ptr<SnapPointWrapper<winrt::ZoomSnapPointBase>> snapPointWrapper : m_sortedConsolidatedZoomSnapPoints)
    {
        winrt::ZoomSnapPointBase winrtZoomSnapPoint = snapPointWrapper->SnapPoint().as<winrt::ZoomSnapPointBase>();

        if (winrtZoomSnapPoint == zoomSnapPoint)
        {
            return snapPointWrapper;
        }
    }

    return nullptr;
}
// End of ScrollPresenterTestHooks calls

// Invoked when a dependency property of this ScrollPresenter has changed.
void ScrollPresenter::OnPropertyChanged(
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    const auto dependencyProperty = args.Property();

    SCROLLPRESENTER_TRACE_VERBOSE_DBG(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToString(dependencyProperty).c_str());

    if (dependencyProperty == s_ContentProperty)
    {
        const winrt::IInspectable oldContent = args.OldValue();
        const winrt::IInspectable newContent = args.NewValue();
        UpdateContent(oldContent.as<winrt::UIElement>(), newContent.as<winrt::UIElement>());
    }
    else if (dependencyProperty == s_BackgroundProperty)
    {
        winrt::Panel thisAsPanel = *this;

        thisAsPanel.Background(args.NewValue().as<winrt::Brush>());
    }
    else if (dependencyProperty == s_MinZoomFactorProperty || dependencyProperty == s_MaxZoomFactorProperty)
    {
        MUX_ASSERT(IsZoomFactorBoundaryValid(unbox_value<double>(args.OldValue())));

        if (m_interactionTracker)
        {
            SetupInteractionTrackerZoomFactorBoundaries(
                MinZoomFactor(),
                MaxZoomFactor());
        }
    }
    else if (dependencyProperty == s_ContentOrientationProperty)
    {
        m_contentOrientation = ContentOrientation();

        InvalidateMeasure();
    }
    else if (dependencyProperty == s_HorizontalAnchorRatioProperty ||
        dependencyProperty == s_VerticalAnchorRatioProperty)
    {
        MUX_ASSERT(IsAnchorRatioValid(unbox_value<double>(args.OldValue())));

        m_isAnchorElementDirty = true;
    }
    else if (m_scrollPresenterVisualInteractionSource)
    {
        if (dependencyProperty == s_HorizontalScrollChainModeProperty)
        {
            SetupVisualInteractionSourceChainingMode(
                m_scrollPresenterVisualInteractionSource,
                ScrollPresenterDimension::HorizontalScroll,
                HorizontalScrollChainMode());
        }
        else if (dependencyProperty == s_VerticalScrollChainModeProperty)
        {
            SetupVisualInteractionSourceChainingMode(
                m_scrollPresenterVisualInteractionSource,
                ScrollPresenterDimension::VerticalScroll,
                VerticalScrollChainMode());
        }
        else if (dependencyProperty == s_ZoomChainModeProperty)
        {
            SetupVisualInteractionSourceChainingMode(
                m_scrollPresenterVisualInteractionSource,
                ScrollPresenterDimension::ZoomFactor,
                ZoomChainMode());
        }
        else if (dependencyProperty == s_HorizontalScrollRailModeProperty)
        {
            SetupVisualInteractionSourceRailingMode(
                m_scrollPresenterVisualInteractionSource,
                ScrollPresenterDimension::HorizontalScroll,
                HorizontalScrollRailMode());
        }
        else if (dependencyProperty == s_VerticalScrollRailModeProperty)
        {
            SetupVisualInteractionSourceRailingMode(
                m_scrollPresenterVisualInteractionSource,
                ScrollPresenterDimension::VerticalScroll,
                VerticalScrollRailMode());
        }
        else if (dependencyProperty == s_HorizontalScrollModeProperty)
        {
            UpdateVisualInteractionSourceMode(
                ScrollPresenterDimension::HorizontalScroll);
        }
        else if (dependencyProperty == s_VerticalScrollModeProperty)
        {
            UpdateVisualInteractionSourceMode(
                ScrollPresenterDimension::VerticalScroll);
        }
        else if (dependencyProperty == s_ZoomModeProperty)
        {
            // Updating the horizontal and vertical scroll modes because GetComputedScrollMode is function of ZoomMode.
            UpdateVisualInteractionSourceMode(
                ScrollPresenterDimension::HorizontalScroll);
            UpdateVisualInteractionSourceMode(
                ScrollPresenterDimension::VerticalScroll);

            SetupVisualInteractionSourceMode(
                m_scrollPresenterVisualInteractionSource,
                ZoomMode());

#ifdef IsMouseWheelZoomDisabled
            SetupVisualInteractionSourcePointerWheelConfig(
                m_scrollPresenterVisualInteractionSource,
                GetMouseWheelZoomMode());
#endif
        }
        else if (dependencyProperty == s_IgnoredInputKindsProperty)
        {
            UpdateManipulationRedirectionMode();
        }
    }
}

void ScrollPresenter::OnContentPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    const winrt::UIElement content = Content();

    if (content)
    {
        if (args == winrt::FrameworkElement::HorizontalAlignmentProperty() ||
            args == winrt::FrameworkElement::VerticalAlignmentProperty())
        {
            // The ExtentWidth and ExtentHeight may have to be updated because of this alignment change.
            InvalidateMeasure();

            if (m_interactionTracker)
            {
                if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
                {
                    SetupPositionBoundariesExpressionAnimations(content);
                }

                if (m_translationExpressionAnimation && m_zoomFactorExpressionAnimation)
                {
                    SetupTransformExpressionAnimations(content);
                }
            }
        }
        else if (args == winrt::FrameworkElement::MinWidthProperty() ||
            args == winrt::FrameworkElement::WidthProperty() ||
            args == winrt::FrameworkElement::MaxWidthProperty() ||
            args == winrt::FrameworkElement::MinHeightProperty() ||
            args == winrt::FrameworkElement::HeightProperty() ||
            args == winrt::FrameworkElement::MaxHeightProperty())
        {
            InvalidateMeasure();
        }
    }
}

void ScrollPresenter::OnFlowDirectionChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_interactionTracker)
    {
        if (winrt::UIElement content = Content())
        {
            if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
            {
                SetupPositionBoundariesExpressionAnimations(content);
            }

            if (m_translationExpressionAnimation && m_zoomFactorExpressionAnimation)
            {
                SetupTransformExpressionAnimations(content);
            }
        }

        if (m_scrollPresenterVisualInteractionSource)
        {
            // When the direction is RightToLeft, the center point modifier is function of the ScrollPresenter width, so it needs to be updated.
            SetupVisualInteractionSourceCenterPointModifier(
                m_scrollPresenterVisualInteractionSource,
                ScrollPresenterDimension::HorizontalScroll,
                true /*flowDirectionChanged*/);
        }

        // The updates above reset the horizontal HorizontalOffset is 0, so it is brought back to its original value through a non-animated scroll.
        if (m_zoomedHorizontalOffset > 0.0)
        {
            com_ptr<ScrollingScrollOptions> options = winrt::make_self<ScrollingScrollOptions>(winrt::ScrollingAnimationMode::Disabled, winrt::ScrollingSnapPointsMode::Ignore);

            ScrollTo(m_zoomedHorizontalOffset, m_zoomedVerticalOffset, *options);
        }
    }
}

void ScrollPresenter::OnCompositionTargetRendering(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    bool unhookCompositionTargetRendering = StartTranslationAndZoomFactorExpressionAnimations();

    if (!m_interactionTrackerAsyncOperations.empty() && IsLoaded())
    {
        bool delayProcessingViewChanges = false;

        for (auto operationsIter = m_interactionTrackerAsyncOperations.begin(); operationsIter != m_interactionTrackerAsyncOperations.end();)
        {
            auto& interactionTrackerAsyncOperation = *operationsIter;

            operationsIter++;

            if (interactionTrackerAsyncOperation->IsDelayed())
            {
                interactionTrackerAsyncOperation->SetIsDelayed(false);
                unhookCompositionTargetRendering = false;
                MUX_ASSERT(interactionTrackerAsyncOperation->IsQueued());
            }
            else if (interactionTrackerAsyncOperation->IsQueued())
            {
                if (!delayProcessingViewChanges && interactionTrackerAsyncOperation->GetTicksCountdown() == 1)
                {
                    // Evaluate whether all remaining queued operations need to be delayed until the completion of a prior required operation.
                    std::shared_ptr<InteractionTrackerAsyncOperation> requiredInteractionTrackerAsyncOperation = interactionTrackerAsyncOperation->GetRequiredOperation();

                    if (requiredInteractionTrackerAsyncOperation)
                    {
                        if (!requiredInteractionTrackerAsyncOperation->IsCanceled() && !requiredInteractionTrackerAsyncOperation->IsCompleted())
                        {
                            // Prior required operation is not canceled or completed yet. All subsequent operations need to be delayed.
                            delayProcessingViewChanges = true;
                        }
                        else
                        {
                            // Previously set required operation is now canceled or completed. Check if it needs to be replaced with an older one.
                            requiredInteractionTrackerAsyncOperation = GetLastNonAnimatedInteractionTrackerOperation(interactionTrackerAsyncOperation);
                            interactionTrackerAsyncOperation->SetRequiredOperation(requiredInteractionTrackerAsyncOperation);
                            if (requiredInteractionTrackerAsyncOperation)
                            {
                                // An older operation is now required. All subsequent operations need to be delayed.
                                delayProcessingViewChanges = true;
                            }
                        }
                    }
                }

                if (delayProcessingViewChanges)
                {
                    if (interactionTrackerAsyncOperation->GetTicksCountdown() > 1)
                    {
                        // Ticking the queued operation without processing it.
                        interactionTrackerAsyncOperation->TickQueuedOperation();
                    }
                    unhookCompositionTargetRendering = false;
                }
                else if (interactionTrackerAsyncOperation->TickQueuedOperation())
                {
                    // InteractionTracker is ready for the operation's processing.
                    ProcessDequeuedViewChange(interactionTrackerAsyncOperation);
                    if (!interactionTrackerAsyncOperation->IsAnimated())
                    {
                        unhookCompositionTargetRendering = false;
                    }
                }
                else
                {
                    unhookCompositionTargetRendering = false;
                }
            }
            else if (!interactionTrackerAsyncOperation->IsAnimated())
            {
                if (interactionTrackerAsyncOperation->TickNonAnimatedOperation())
                {
                    // The non-animated view change request did not result in a status change or ValuesChanged notification. Consider it completed.
                    CompleteViewChange(interactionTrackerAsyncOperation, ScrollPresenterViewChangeResult::Completed);
                    if (m_translationAndZoomFactorAnimationsRestartTicksCountdown > 0)
                    {
                        // Do not unhook the Rendering event when there is a pending restart of the Translation and Scale animations.
                        unhookCompositionTargetRendering = false;
                    }
                    m_interactionTrackerAsyncOperations.remove(interactionTrackerAsyncOperation);

                    if (m_interactionTrackerAsyncOperations.empty())
                    {
                        ResetAnticipatedView();
                    }
                }
                else
                {
                    unhookCompositionTargetRendering = false;
                }
            }
        }
    }

    if (unhookCompositionTargetRendering)
    {
        UnhookCompositionTargetRendering();
    }
}

void ScrollPresenter::OnLoaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    SetupInteractionTrackerBoundaries();

    EnsureScrollPresenterVisualInteractionSource();
    SetupScrollPresenterVisualInteractionSource();
    SetupScrollControllerVisualInterationSource(ScrollPresenterDimension::HorizontalScroll);
    SetupScrollControllerVisualInterationSource(ScrollPresenterDimension::VerticalScroll);

    if (m_horizontalScrollControllerExpressionAnimationSources)
    {
        MUX_ASSERT(m_horizontalScrollControllerPanningInfo);

        m_horizontalScrollControllerPanningInfo.get().SetPanningElementExpressionAnimationSources(
            m_horizontalScrollControllerExpressionAnimationSources,
            s_minOffsetPropertyName,
            s_maxOffsetPropertyName,
            s_offsetPropertyName,
            s_multiplierPropertyName);
    }
    if (m_verticalScrollControllerExpressionAnimationSources)
    {
        MUX_ASSERT(m_verticalScrollControllerPanningInfo);

        m_verticalScrollControllerPanningInfo.get().SetPanningElementExpressionAnimationSources(
            m_verticalScrollControllerExpressionAnimationSources,
            s_minOffsetPropertyName,
            s_maxOffsetPropertyName,
            s_offsetPropertyName,
            s_multiplierPropertyName);
    }

    const winrt::UIElement content = Content();

    if (content)
    {
        if (!m_translationExpressionAnimation || !m_zoomFactorExpressionAnimation)
        {
            EnsureTransformExpressionAnimations();
            SetupTransformExpressionAnimations(content);
        }

        // Process the potentially delayed operation in the OnCompositionTargetRendering handler.
        HookCompositionTargetRendering();
    }
}

void ScrollPresenter::OnUnloaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!IsLoaded())
    {
        MUX_ASSERT(RenderSize().Width == 0.0);
        MUX_ASSERT(RenderSize().Height == 0.0);

        // All potential pending operations are interrupted when the ScrollPresenter unloads.
        CompleteInteractionTrackerOperations(
            -1 /*requestId*/,
            ScrollPresenterViewChangeResult::Interrupted /*operationResult*/,
            ScrollPresenterViewChangeResult::Ignored     /*unused priorNonAnimatedOperationsResult*/,
            ScrollPresenterViewChangeResult::Ignored     /*unused priorAnimatedOperationsResult*/,
            true  /*completeNonAnimatedOperation*/,
            true  /*completeAnimatedOperation*/,
            false /*completePriorNonAnimatedOperations*/,
            false /*completePriorAnimatedOperations*/);

        // Unhook the potential OnCompositionTargetRendering handler since there are no pending operations.
        UnhookCompositionTargetRendering();

        const winrt::UIElement content = Content();

        UpdateUnzoomedExtentAndViewport(
            false /*renderSizeChanged*/,
            content ? m_unzoomedExtentWidth : 0.0,
            content ? m_unzoomedExtentHeight : 0.0,
            0.0 /*viewportWidth*/,
            0.0 /*viewportHeight*/);
    }
}

// UIElement.BringIntoViewRequested event handler to bring an element into the viewport.
void ScrollPresenter::OnBringIntoViewRequestedHandler(
    const winrt::IInspectable& /*sender*/,
    const winrt::BringIntoViewRequestedEventArgs& args)
{
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, L"%s[0x%p](AnimationDesired:%d, Handled:%d, H/V AlignmentRatio:%lf,%lf, H/V Offset:%f,%f, TargetRect:%s, TargetElement:0x%p)\n",
        METH_NAME, this,
        args.AnimationDesired(), args.Handled(),
        args.HorizontalAlignmentRatio(), args.VerticalAlignmentRatio(),
        args.HorizontalOffset(), args.VerticalOffset(),
        TypeLogging::RectToString(args.TargetRect()).c_str(), args.TargetElement());

    winrt::UIElement content = Content();

    if (args.Handled() ||
        args.TargetElement() == static_cast<winrt::UIElement>(*this) ||
        (args.TargetElement() == content && content.Visibility() == winrt::Visibility::Collapsed) ||
        (args.TargetElement() != content && !SharedHelpers::IsAncestor(args.TargetElement(), content, true /*checkVisibility*/)))
    {
        // Ignore the request when:
        // - There is no InteractionTracker to fulfill it.
        // - It was handled already.
        // - The target element is this ScrollPresenter itself. A parent scrollPresenter may fulfill the request instead then.
        // - The target element is effectively collapsed within the ScrollPresenter.
        return;
    }

    winrt::Rect targetRect{};
    int32_t offsetsChangeCorrelationId = s_noOpCorrelationId;
    double targetZoomedHorizontalOffset = 0.0;
    double targetZoomedVerticalOffset = 0.0;
    double appliedOffsetX = 0.0;
    double appliedOffsetY = 0.0;
    winrt::ScrollingSnapPointsMode snapPointsMode = winrt::ScrollingSnapPointsMode::Ignore;

    // Compute the target offsets based on the provided BringIntoViewRequestedEventArgs.
    ComputeBringIntoViewTargetOffsetsFromRequestEventArgs(
        content,
        snapPointsMode,
        args,
        &targetZoomedHorizontalOffset,
        &targetZoomedVerticalOffset,
        &appliedOffsetX,
        &appliedOffsetY,
        &targetRect);

    if (HasBringingIntoViewListener())
    {
        // Raise the ScrollPresenter.BringingIntoView event to give the listeners a chance to adjust the operation.

        offsetsChangeCorrelationId = m_latestViewChangeCorrelationId = GetNextViewChangeCorrelationId();

        if (!RaiseBringingIntoView(
            targetZoomedHorizontalOffset,
            targetZoomedVerticalOffset,
            args,
            offsetsChangeCorrelationId,
            &snapPointsMode))
        {
            // A listener canceled the operation in the ScrollPresenter.BringingIntoView event handler before any scrolling was attempted.
            RaiseViewChangeCompleted(true /*isForScroll*/, ScrollPresenterViewChangeResult::Completed, offsetsChangeCorrelationId);
            return;
        }

        content = Content();

        if (!content ||
            args.Handled() ||
            args.TargetElement() == static_cast<winrt::UIElement>(*this) ||
            (args.TargetElement() == content && content.Visibility() == winrt::Visibility::Collapsed) ||
            (args.TargetElement() != content && !SharedHelpers::IsAncestor(args.TargetElement(), content, true /*checkVisibility*/)))
        {
            // Again, ignore the request when:
            // - There is no Content anymore.
            // - The request was handled already.
            // - The target element is this ScrollPresenter itself. A parent scrollPresenter may fulfill the request instead then.
            // - The target element is effectively collapsed within the ScrollPresenter.
            return;
        }

        // Re-evaluate the target offsets based on the potentially modified BringIntoViewRequestedEventArgs.
        // Take into account potential SnapPointsMode == Default so that parents contribute accordingly.
        ComputeBringIntoViewTargetOffsetsFromRequestEventArgs(
            content,
            snapPointsMode,
            args,
            &targetZoomedHorizontalOffset,
            &targetZoomedVerticalOffset,
            &appliedOffsetX,
            &appliedOffsetY,
            &targetRect);
    }

    // Do not include the applied offsets so that potential parent bring-into-view contributors ignore that shift.
    const winrt::Rect nextTargetRect{
        static_cast<float>(targetRect.X * m_zoomFactor - targetZoomedHorizontalOffset - appliedOffsetX),
        static_cast<float>(targetRect.Y * m_zoomFactor - targetZoomedVerticalOffset - appliedOffsetY),
        std::min(targetRect.Width * m_zoomFactor, static_cast<float>(m_viewportWidth)),
        std::min(targetRect.Height * m_zoomFactor, static_cast<float>(m_viewportHeight))
    };

    const winrt::Rect viewportRect{
        0.0f,
        0.0f,
        static_cast<float>(m_viewportWidth),
        static_cast<float>(m_viewportHeight),
    };

    if (targetZoomedHorizontalOffset != m_zoomedHorizontalOffset ||
        targetZoomedVerticalOffset != m_zoomedVerticalOffset)
    {
        com_ptr<ScrollingScrollOptions> options =
            winrt::make_self<ScrollingScrollOptions>(
                args.AnimationDesired() ? winrt::ScrollingAnimationMode::Auto : winrt::ScrollingAnimationMode::Disabled,
                snapPointsMode);

        ChangeOffsetsPrivate(
            targetZoomedHorizontalOffset /*zoomedHorizontalOffset*/,
            targetZoomedVerticalOffset /*zoomedVerticalOffset*/,
            ScrollPresenterViewKind::Absolute,
            *options,
            args /*bringIntoViewRequestedEventArgs*/,
            InteractionTrackerAsyncOperationTrigger::BringIntoViewRequest,
            offsetsChangeCorrelationId /*existingViewChangeCorrelationId*/,
            nullptr /*viewChangeCorrelationId*/);
    }
    else
    {
        // No offset change was triggered because the target offsets are the same as the current ones. Mark the operation as completed immediately.
        RaiseViewChangeCompleted(true /*isForScroll*/, ScrollPresenterViewChangeResult::Completed, offsetsChangeCorrelationId);
    }

    if (SharedHelpers::DoRectsIntersect(nextTargetRect, viewportRect))
    {
        // Next bring a portion of this ScrollPresenter into view.
        args.TargetRect(nextTargetRect);
        args.TargetElement(*this);
        args.HorizontalOffset(args.HorizontalOffset() - appliedOffsetX);
        args.VerticalOffset(args.VerticalOffset() - appliedOffsetY);
    }
    else
    {
        // This ScrollPresenter did not even partially bring the TargetRect into its viewport.
        // Mark the operation as handled since no portion of this ScrollPresenter needs to be brought into view.
        args.Handled(true);
    }
}

void ScrollPresenter::OnPointerPressed(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(m_scrollPresenterVisualInteractionSource);

    if (m_horizontalScrollController && m_horizontalScrollController.get().IsScrollingWithMouse())
    {
        return;
    }

    if (m_verticalScrollController && m_verticalScrollController.get().IsScrollingWithMouse())
    {
        return;
    }

    const winrt::UIElement content = Content();
    const winrt::ScrollingScrollMode horizontalScrollMode = GetComputedScrollMode(ScrollPresenterDimension::HorizontalScroll);
    const winrt::ScrollingScrollMode verticalScrollMode = GetComputedScrollMode(ScrollPresenterDimension::VerticalScroll);

    if (!content ||
        (horizontalScrollMode == winrt::ScrollingScrollMode::Disabled &&
            verticalScrollMode == winrt::ScrollingScrollMode::Disabled &&
            ZoomMode() == winrt::ScrollingZoomMode::Disabled))
    {
        return;
    }

    switch (args.Pointer().PointerDeviceType())
    {
    case winrt::PointerDeviceType::Touch:
        if (IsInputKindIgnored(winrt::ScrollingInputKinds::Touch))
            return;
        break;
    case winrt::PointerDeviceType::Pen:
        if (IsInputKindIgnored(winrt::ScrollingInputKinds::Pen))
            return;
        break;
    default:
        return;
    }

    // All UIElement instances between the touched one and the ScrollPresenter must include ManipulationModes.System in their
    // ManipulationMode property in order to trigger a manipulation. This allows to turn off touch interactions in particular.
    winrt::IInspectable source = args.OriginalSource();
    MUX_ASSERT(source);

    winrt::DependencyObject sourceAsDO = source.try_as<winrt::DependencyObject>();

    winrt::IUIElement thisAsUIElement = *this; // Need to have exactly the same interface as we're comparing below for object equality

    while (sourceAsDO)
    {
        winrt::IUIElement sourceAsUIE = sourceAsDO.try_as<winrt::IUIElement>();
        if (sourceAsUIE)
        {
            const winrt::ManipulationModes mm = sourceAsUIE.ManipulationMode();

            if ((mm & winrt::ManipulationModes::System) == winrt::ManipulationModes::None)
            {
                return;
            }

            if (sourceAsUIE == thisAsUIElement)
            {
                break;
            }
        }

        sourceAsDO = winrt::VisualTreeHelper::GetParent(sourceAsDO);
    };

#ifdef DBG
    DumpMinMaxPositions();

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_STR, METH_NAME, this, L"TryRedirectForManipulation", TypeLogging::PointerPointToString(args.GetCurrentPoint(nullptr)).c_str());
#endif // DBG

    try
    {
        m_scrollPresenterVisualInteractionSource.TryRedirectForManipulation(args.GetCurrentPoint(nullptr));
    }
    catch (const winrt::hresult_error & e)
    {
        // Swallowing Access Denied error because of InteractionTracker bug 17434718 which has been
        // causing crashes at least in RS3, RS4 and RS5.
        // TODO - Stop eating the error in future OS versions that include a fix for 17434718 if any.
        if (e.to_abi() != E_ACCESSDENIED)
        {
            throw;
        }
    }
}

// Invoked by an IScrollControllerPanningInfo implementation when a call to InteractionTracker::TryRedirectForManipulation
// is required to track a finger.
void ScrollPresenter::OnScrollControllerPanningInfoPanRequested(
    const winrt::IScrollControllerPanningInfo& sender,
    const winrt::ScrollControllerPanRequestedEventArgs& args)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);

    MUX_ASSERT(sender == m_horizontalScrollControllerPanningInfo.get() || sender == m_verticalScrollControllerPanningInfo.get());

    if (args.Handled())
    {
        return;
    }

    winrt::VisualInteractionSource scrollControllerVisualInteractionSource = nullptr;

    if (sender == m_horizontalScrollControllerPanningInfo.get())
    {
        scrollControllerVisualInteractionSource = m_horizontalScrollControllerVisualInteractionSource;
    }
    else
    {
        scrollControllerVisualInteractionSource = m_verticalScrollControllerVisualInteractionSource;
    }

    if (scrollControllerVisualInteractionSource)
    {
        try
        {
            scrollControllerVisualInteractionSource.TryRedirectForManipulation(args.PointerPoint());
        }
        catch (const winrt::hresult_error & e)
        {
            // Swallowing Access Denied error because of InteractionTracker bug 17434718 which has been
            // causing crashes at least in RS3, RS4 and RS5.
            // TODO - Stop eating the error in future OS versions that include a fix for 17434718 if any.
            if (e.to_abi() == E_ACCESSDENIED)
            {
                // Do not set the Handled flag. The request is simply ignored.
                return;
            }
            else
            {
                throw;
            }
        }
        args.Handled(true);
    }
}

// Invoked by an IScrollControllerPanningInfo implementation when one or more of its characteristics has changed:
// PanningElementAncestor, PanOrientation or IsRailEnabled.
void ScrollPresenter::OnScrollControllerPanningInfoChanged(
    const winrt::IScrollControllerPanningInfo& sender,
    const winrt::IInspectable& /*args*/)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);

    MUX_ASSERT(sender == m_horizontalScrollControllerPanningInfo.get() || sender == m_verticalScrollControllerPanningInfo.get());

    if (!m_interactionTracker)
    {
        return;
    }

    const bool isFromHorizontalScrollController = sender == m_horizontalScrollControllerPanningInfo.get();

    winrt::CompositionPropertySet scrollControllerExpressionAnimationSources =
        isFromHorizontalScrollController ? m_horizontalScrollControllerExpressionAnimationSources : m_verticalScrollControllerExpressionAnimationSources;

    SetupScrollControllerVisualInterationSource(isFromHorizontalScrollController ? ScrollPresenterDimension::HorizontalScroll : ScrollPresenterDimension::VerticalScroll);

    if (isFromHorizontalScrollController)
    {
        if (scrollControllerExpressionAnimationSources != m_horizontalScrollControllerExpressionAnimationSources)
        {
            MUX_ASSERT(m_horizontalScrollControllerPanningInfo);

            m_horizontalScrollControllerPanningInfo.get().SetPanningElementExpressionAnimationSources(
                m_horizontalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }
    else
    {
        if (scrollControllerExpressionAnimationSources != m_verticalScrollControllerExpressionAnimationSources)
        {
            MUX_ASSERT(m_verticalScrollControllerPanningInfo);

            m_verticalScrollControllerPanningInfo.get().SetPanningElementExpressionAnimationSources(
                m_verticalScrollControllerExpressionAnimationSources,
                s_minOffsetPropertyName,
                s_maxOffsetPropertyName,
                s_offsetPropertyName,
                s_multiplierPropertyName);
        }
    }
}

// Invoked when a IScrollController::ScrollToRequested event is raised in order to perform the
// equivalent of a ScrollPresenter::ScrollTo operation.
void ScrollPresenter::OnScrollControllerScrollToRequested(
    const winrt::IScrollController& sender,
    const winrt::ScrollControllerScrollToRequestedEventArgs& args)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);
    MUX_ASSERT(sender == m_horizontalScrollController.get() || sender == m_verticalScrollController.get());

    const bool isFromHorizontalScrollController = sender == m_horizontalScrollController.get();
    int32_t viewChangeCorrelationId = s_noOpCorrelationId;

    // Attempt to find an offset change request from an IScrollController with the same ScrollPresenterViewKind,
    // the same ScrollingScrollOptions settings and same tick.
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationFromKinds(
        true /*isOperationTypeForOffsetsChange*/,
        static_cast<InteractionTrackerAsyncOperationTrigger>(static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) + static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest)),
        ScrollPresenterViewKind::Absolute,
        args.Options());

    if (!interactionTrackerAsyncOperation)
    {
        ChangeOffsetsPrivate(
            isFromHorizontalScrollController ? args.Offset() : m_zoomedHorizontalOffset,
            isFromHorizontalScrollController ? m_zoomedVerticalOffset : args.Offset(),
            ScrollPresenterViewKind::Absolute,
            args.Options(),
            nullptr /*bringIntoViewRequestedEventArgs*/,
            isFromHorizontalScrollController ? InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest : InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest,
            s_noOpCorrelationId /*existingViewChangeCorrelationId*/,
            &viewChangeCorrelationId);
    }
    else
    {
        // Coalesce requests
        const int32_t existingViewChangeCorrelationId = interactionTrackerAsyncOperation->GetViewChangeCorrelationId();
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();
        std::shared_ptr<OffsetsChange> offsetsChange = std::reinterpret_pointer_cast<OffsetsChange>(viewChangeBase);

        interactionTrackerAsyncOperation->SetIsScrollControllerRequest(isFromHorizontalScrollController);

        if (isFromHorizontalScrollController)
        {
            offsetsChange->ZoomedHorizontalOffset(args.Offset());
        }
        else
        {
            offsetsChange->ZoomedVerticalOffset(args.Offset());
        }

        viewChangeCorrelationId = existingViewChangeCorrelationId;
    }

    if (viewChangeCorrelationId != s_noOpCorrelationId)
    {
        args.CorrelationId(viewChangeCorrelationId);
    }
}

// Invoked when a IScrollController::ScrollByRequested event is raised in order to perform the
// equivalent of a ScrollPresenter::ScrollBy operation.
void ScrollPresenter::OnScrollControllerScrollByRequested(
    const winrt::IScrollController& sender,
    const winrt::ScrollControllerScrollByRequestedEventArgs& args)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);
    MUX_ASSERT(sender == m_horizontalScrollController.get() || sender == m_verticalScrollController.get());

    const bool isFromHorizontalScrollController = sender == m_horizontalScrollController.get();
    int32_t viewChangeCorrelationId = s_noOpCorrelationId;

    // Attempt to find an offset change request from an IScrollController with the same ScrollPresenterViewKind,
    // the same ScrollingScrollOptions settings and same tick.
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationFromKinds(
        true /*isOperationTypeForOffsetsChange*/,
        static_cast<InteractionTrackerAsyncOperationTrigger>(static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) + static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest)),
        ScrollPresenterViewKind::RelativeToCurrentView,
        args.Options());

    if (!interactionTrackerAsyncOperation)
    {
        ChangeOffsetsPrivate(
            isFromHorizontalScrollController ? args.OffsetDelta() : 0.0 /*zoomedHorizontalOffset*/,
            isFromHorizontalScrollController ? 0.0 : args.OffsetDelta() /*zoomedVerticalOffset*/,
            ScrollPresenterViewKind::RelativeToCurrentView,
            args.Options(),
            nullptr /*bringIntoViewRequestedEventArgs*/,
            isFromHorizontalScrollController ? InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest : InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest,
            s_noOpCorrelationId /*existingViewChangeCorrelationId*/,
            &viewChangeCorrelationId);
    }
    else
    {
        // Coalesce requests
        const int32_t existingViewChangeCorrelationId = interactionTrackerAsyncOperation->GetViewChangeCorrelationId();
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();
        std::shared_ptr<OffsetsChange> offsetsChange = std::reinterpret_pointer_cast<OffsetsChange>(viewChangeBase);

        interactionTrackerAsyncOperation->SetIsScrollControllerRequest(isFromHorizontalScrollController);

        if (isFromHorizontalScrollController)
        {
            offsetsChange->ZoomedHorizontalOffset(offsetsChange->ZoomedHorizontalOffset() + args.OffsetDelta());
        }
        else
        {
            offsetsChange->ZoomedVerticalOffset(offsetsChange->ZoomedVerticalOffset() + args.OffsetDelta());
        }

        viewChangeCorrelationId = existingViewChangeCorrelationId;
    }

    if (viewChangeCorrelationId != s_noOpCorrelationId)
    {
        args.CorrelationId(viewChangeCorrelationId);
    }
}

// Invoked when a IScrollController::AddScrollVelocityRequested event is raised in order to perform the
// equivalent of a ScrollPresenter::AddScrollVelocity operation.
void ScrollPresenter::OnScrollControllerAddScrollVelocityRequested(
    const winrt::IScrollController& sender,
    const winrt::ScrollControllerAddScrollVelocityRequestedEventArgs& args)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, sender);
    MUX_ASSERT(sender == m_horizontalScrollController.get() || sender == m_verticalScrollController.get());

    const bool isFromHorizontalScrollController = sender == m_horizontalScrollController.get();
    int32_t viewChangeCorrelationId = s_noOpCorrelationId;
    winrt::IReference<float> horizontalInertiaDecayRate = nullptr;
    winrt::IReference<float> verticalInertiaDecayRate = nullptr;

    // Attempt to find an offset change with velocity request from an IScrollController and this same tick.
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation = GetInteractionTrackerOperationWithAdditionalVelocity(
        true /*isOperationTypeForOffsetsChange*/,
        static_cast<InteractionTrackerAsyncOperationTrigger>(static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) + static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest)));

    if (!interactionTrackerAsyncOperation)
    {
        winrt::IReference<winrt::float2> inertiaDecayRate = nullptr;
        winrt::float2 offsetsVelocity{};

        if (isFromHorizontalScrollController)
        {
            offsetsVelocity.x = args.OffsetVelocity();
            horizontalInertiaDecayRate = args.InertiaDecayRate();
        }
        else
        {
            offsetsVelocity.y = args.OffsetVelocity();
            verticalInertiaDecayRate = args.InertiaDecayRate();
        }

        if (horizontalInertiaDecayRate || verticalInertiaDecayRate)
        {
            winrt::IInspectable inertiaDecayRateAsInsp = nullptr;

            if (horizontalInertiaDecayRate)
            {
                inertiaDecayRateAsInsp = box_value(winrt::float2({ horizontalInertiaDecayRate.Value(), c_scrollPresenterDefaultInertiaDecayRate }));
            }
            else
            {
                inertiaDecayRateAsInsp = box_value(winrt::float2({ c_scrollPresenterDefaultInertiaDecayRate, verticalInertiaDecayRate.Value() }));
            }

            inertiaDecayRate = inertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();
        }

        ChangeOffsetsWithAdditionalVelocityPrivate(
            offsetsVelocity,
            winrt::float2::zero() /*anticipatedOffsetsChange*/,
            inertiaDecayRate,
            isFromHorizontalScrollController ? InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest : InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest,
            &viewChangeCorrelationId);
    }
    else
    {
        // Coalesce requests
        const int32_t existingViewChangeCorrelationId = interactionTrackerAsyncOperation->GetViewChangeCorrelationId();
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();
        std::shared_ptr<OffsetsChangeWithAdditionalVelocity> offsetsChangeWithAdditionalVelocity = std::reinterpret_pointer_cast<OffsetsChangeWithAdditionalVelocity>(viewChangeBase);

        winrt::float2 offsetsVelocity = offsetsChangeWithAdditionalVelocity->OffsetsVelocity();
        winrt::IReference<winrt::float2> inertiaDecayRate = offsetsChangeWithAdditionalVelocity->InertiaDecayRate();

        interactionTrackerAsyncOperation->SetIsScrollControllerRequest(isFromHorizontalScrollController);

        if (isFromHorizontalScrollController)
        {
            offsetsVelocity.x = args.OffsetVelocity();
            horizontalInertiaDecayRate = args.InertiaDecayRate();

            if (!horizontalInertiaDecayRate)
            {
                if (inertiaDecayRate)
                {
                    if (inertiaDecayRate.Value().y == c_scrollPresenterDefaultInertiaDecayRate)
                    {
                        offsetsChangeWithAdditionalVelocity->InertiaDecayRate(nullptr);
                    }
                    else
                    {
                        winrt::IInspectable newInertiaDecayRateAsInsp =
                            box_value(winrt::float2({ c_scrollPresenterDefaultInertiaDecayRate, inertiaDecayRate.Value().y }));
                        winrt::IReference<winrt::float2> newInertiaDecayRate =
                            newInertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();

                        offsetsChangeWithAdditionalVelocity->InertiaDecayRate(newInertiaDecayRate);
                    }
                }
            }
            else
            {
                winrt::IInspectable newInertiaDecayRateAsInsp = nullptr;

                if (!inertiaDecayRate)
                {
                    newInertiaDecayRateAsInsp =
                        box_value(winrt::float2({ horizontalInertiaDecayRate.Value(), c_scrollPresenterDefaultInertiaDecayRate }));
                }
                else
                {
                    newInertiaDecayRateAsInsp =
                        box_value(winrt::float2({ horizontalInertiaDecayRate.Value(), inertiaDecayRate.Value().y }));
                }

                winrt::IReference<winrt::float2> newInertiaDecayRate = newInertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();

                offsetsChangeWithAdditionalVelocity->InertiaDecayRate(newInertiaDecayRate);
            }
        }
        else
        {
            offsetsVelocity.y = args.OffsetVelocity();
            verticalInertiaDecayRate = args.InertiaDecayRate();

            if (!verticalInertiaDecayRate)
            {
                if (inertiaDecayRate)
                {
                    if (inertiaDecayRate.Value().x == c_scrollPresenterDefaultInertiaDecayRate)
                    {
                        offsetsChangeWithAdditionalVelocity->InertiaDecayRate(nullptr);
                    }
                    else
                    {
                        winrt::IInspectable newInertiaDecayRateAsInsp =
                            box_value(winrt::float2({ inertiaDecayRate.Value().x, c_scrollPresenterDefaultInertiaDecayRate }));
                        winrt::IReference<winrt::float2> newInertiaDecayRate =
                            newInertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();

                        offsetsChangeWithAdditionalVelocity->InertiaDecayRate(newInertiaDecayRate);
                    }
                }
            }
            else
            {
                winrt::IInspectable newInertiaDecayRateAsInsp = nullptr;

                if (!inertiaDecayRate)
                {
                    newInertiaDecayRateAsInsp =
                        box_value(winrt::float2({ c_scrollPresenterDefaultInertiaDecayRate, verticalInertiaDecayRate.Value() }));
                }
                else
                {
                    newInertiaDecayRateAsInsp =
                        box_value(winrt::float2({ inertiaDecayRate.Value().x, verticalInertiaDecayRate.Value() }));
                }

                winrt::IReference<winrt::float2> newInertiaDecayRate = newInertiaDecayRateAsInsp.as<winrt::IReference<winrt::float2>>();

                offsetsChangeWithAdditionalVelocity->InertiaDecayRate(newInertiaDecayRate);
            }
        }

        offsetsChangeWithAdditionalVelocity->OffsetsVelocity(offsetsVelocity);

        viewChangeCorrelationId = existingViewChangeCorrelationId;
    }

    if (viewChangeCorrelationId != s_noOpCorrelationId)
    {
        args.CorrelationId(viewChangeCorrelationId);
    }
}

void ScrollPresenter::OnHorizontalSnapPointsVectorChanged(const winrt::IObservableVector<winrt::ScrollSnapPointBase>& sender, const winrt::IVectorChangedEventArgs args)
{
    SnapPointsVectorChangedHelper(sender, args, &m_sortedConsolidatedHorizontalSnapPoints, ScrollPresenterDimension::HorizontalScroll);
}

void ScrollPresenter::OnVerticalSnapPointsVectorChanged(const winrt::IObservableVector<winrt::ScrollSnapPointBase>& sender, const winrt::IVectorChangedEventArgs args)
{
    SnapPointsVectorChangedHelper(sender, args, &m_sortedConsolidatedVerticalSnapPoints, ScrollPresenterDimension::VerticalScroll);
}

void ScrollPresenter::OnZoomSnapPointsVectorChanged(const winrt::IObservableVector<winrt::ZoomSnapPointBase>& sender, const winrt::IVectorChangedEventArgs args)
{
    SnapPointsVectorChangedHelper(sender, args, &m_sortedConsolidatedZoomSnapPoints, ScrollPresenterDimension::ZoomFactor);
}

template <typename T>
bool ScrollPresenter::SnapPointsViewportChangedHelper(
    winrt::IObservableVector<T> const& snapPoints,
    double viewport)
{
    bool snapPointsNeedViewportUpdates = false;

    for (T snapPoint : snapPoints)
    {
        winrt::SnapPointBase winrtSnapPointBase = snapPoint.as<winrt::SnapPointBase>();
        SnapPointBase* snapPointBase = winrt::get_self<SnapPointBase>(winrtSnapPointBase);

        snapPointsNeedViewportUpdates |= snapPointBase->OnUpdateViewport(viewport);
    }

    return snapPointsNeedViewportUpdates;
}

template <typename T>
void ScrollPresenter::SnapPointsVectorChangedHelper(
    winrt::IObservableVector<T> const& snapPoints,
    winrt::IVectorChangedEventArgs const& args,
    std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet,
    ScrollPresenterDimension dimension)
{
    MUX_ASSERT(snapPoints);
    MUX_ASSERT(snapPointsSet);

    T insertedItem = nullptr;
    const winrt::CollectionChange collectionChange = args.CollectionChange();

    if (dimension != ScrollPresenterDimension::ZoomFactor)
    {
        const double viewportSize = dimension == ScrollPresenterDimension::HorizontalScroll ? m_viewportWidth : m_viewportHeight;

        if (collectionChange == winrt::CollectionChange::ItemInserted)
        {
            insertedItem = snapPoints.GetAt(args.Index());

            winrt::SnapPointBase winrtSnapPointBase = insertedItem.as<winrt::SnapPointBase>();
            SnapPointBase* snapPointBase = winrt::get_self<SnapPointBase>(winrtSnapPointBase);

            // Newly inserted scroll snap point is provided the viewport size, for the case it's not near-aligned.
            const bool snapPointNeedsViewportUpdates = snapPointBase->OnUpdateViewport(viewportSize);

            // When snapPointNeedsViewportUpdates is True, this newly inserted scroll snap point may be the first one
            // that requires viewport updates.
            if (dimension == ScrollPresenterDimension::HorizontalScroll)
            {
                m_horizontalSnapPointsNeedViewportUpdates |= snapPointNeedsViewportUpdates;
            }
            else
            {
                m_verticalSnapPointsNeedViewportUpdates |= snapPointNeedsViewportUpdates;
            }
        }
        else if (collectionChange == winrt::CollectionChange::Reset ||
            collectionChange == winrt::CollectionChange::ItemChanged)
        {
            // Globally reevaluate the need for viewport updates even for CollectionChange::ItemChanged since
            // the old item may or may not have been the sole snap point requiring viewport updates.
            const bool snapPointsNeedViewportUpdates = SnapPointsViewportChangedHelper(snapPoints, viewportSize);

            if (dimension == ScrollPresenterDimension::HorizontalScroll)
            {
                m_horizontalSnapPointsNeedViewportUpdates = snapPointsNeedViewportUpdates;
            }
            else
            {
                m_verticalSnapPointsNeedViewportUpdates = snapPointsNeedViewportUpdates;
            }
        }
    }

    switch (collectionChange)
    {
    case winrt::CollectionChange::ItemInserted:
    {
        if (!insertedItem)
        {
            insertedItem = snapPoints.GetAt(args.Index());
        }

        std::shared_ptr<SnapPointWrapper<T>> insertedSnapPointWrapper =
            std::make_shared<SnapPointWrapper<T>>(insertedItem);

        SnapPointsVectorItemInsertedHelper(insertedSnapPointWrapper, snapPointsSet);
        break;
    }
    case winrt::CollectionChange::Reset:
    case winrt::CollectionChange::ItemRemoved:
    case winrt::CollectionChange::ItemChanged:
    {
        RegenerateSnapPointsSet(snapPoints, snapPointsSet);
        break;
    }
    default:
        MUX_ASSERT(false);
        break;
    }

    SetupSnapPoints(snapPointsSet, dimension);
}

template <typename T>
void ScrollPresenter::SnapPointsVectorItemInsertedHelper(
    std::shared_ptr<SnapPointWrapper<T>> insertedItem,
    std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* snapPointsSet)
{
    if (snapPointsSet->empty())
    {
        snapPointsSet->insert(insertedItem);
        return;
    }

    winrt::SnapPointBase winrtInsertedItem = insertedItem->SnapPoint().as<winrt::SnapPointBase>();
    auto lowerBound = snapPointsSet->lower_bound(insertedItem);

    if (lowerBound != snapPointsSet->end())
    {
        winrt::SnapPointBase winrtSnapPointBase = (*lowerBound)->SnapPoint().as<winrt::SnapPointBase>();
        SnapPointBase* lowerSnapPoint = winrt::get_self<SnapPointBase>(winrtSnapPointBase);

        if (*lowerSnapPoint == winrt::get_self<SnapPointBase>(winrtInsertedItem))
        {
            (*lowerBound)->Combine(insertedItem.get());
            return;
        }
        lowerBound++;
    }
    if (lowerBound != snapPointsSet->end())
    {
        winrt::SnapPointBase winrtSnapPointBase = (*lowerBound)->SnapPoint().as<winrt::SnapPointBase>();
        SnapPointBase* upperSnapPoint = winrt::get_self<SnapPointBase>(winrtSnapPointBase);

        if (*upperSnapPoint == winrt::get_self<SnapPointBase>(winrtInsertedItem))
        {
            (*lowerBound)->Combine(insertedItem.get());
            return;
        }
    }
    snapPointsSet->insert(insertedItem);
}

template <typename T>
void ScrollPresenter::RegenerateSnapPointsSet(
    winrt::IObservableVector<T> const& userVector,
    std::set<std::shared_ptr<SnapPointWrapper<T>>, SnapPointWrapperComparator<T>>* internalSet)
{
    MUX_ASSERT(internalSet);

    internalSet->clear();
    for (T snapPoint : userVector)
    {
        std::shared_ptr<SnapPointWrapper<T>> snapPointWrapper =
            std::make_shared<SnapPointWrapper<T>>(snapPoint);

        SnapPointsVectorItemInsertedHelper(snapPointWrapper, internalSet);
    }
}

void ScrollPresenter::UpdateContent(
    const winrt::UIElement& oldContent,
    const winrt::UIElement& newContent)
{
    auto children = Children();
    children.Clear();

    UnhookContentPropertyChanged(oldContent);

    if (newContent)
    {
        children.Append(newContent);

        if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
        {
            UpdatePositionBoundaries(newContent);
        }
        else if (m_interactionTracker)
        {
            EnsurePositionBoundariesExpressionAnimations();
            SetupPositionBoundariesExpressionAnimations(newContent);
        }

        if (m_translationExpressionAnimation && m_zoomFactorExpressionAnimation)
        {
            UpdateTransformSource(oldContent, newContent);
        }
        else if (m_interactionTracker)
        {
            EnsureTransformExpressionAnimations();
            SetupTransformExpressionAnimations(newContent);
        }

        HookContentPropertyChanged(newContent);
    }
    else
    {
        UpdateUnzoomedExtentAndViewport(
            false /*renderSizeChanged*/,
            0.0 /*unzoomedExtentWidth*/,
            0.0 /*unzoomedExtentHeight*/,
            m_viewportWidth,
            m_viewportHeight);

        if (m_contentLayoutOffsetX != 0.0f)
        {
            m_contentLayoutOffsetX = 0.0f;
            OnContentLayoutOffsetChanged(ScrollPresenterDimension::HorizontalScroll);
        }

        if (m_contentLayoutOffsetY != 0.0f)
        {
            m_contentLayoutOffsetY = 0.0f;
            OnContentLayoutOffsetChanged(ScrollPresenterDimension::VerticalScroll);
        }

        if (!m_interactionTracker || (m_zoomedHorizontalOffset == 0.0 && m_zoomedVerticalOffset == 0.0))
        {
            // Complete all active or delayed operations when there is no InteractionTracker, when the old content
            // was already at offsets (0,0). The ScrollToOffsets request below will result in their completion otherwise.
            CompleteInteractionTrackerOperations(
                -1 /*requestId*/,
                ScrollPresenterViewChangeResult::Interrupted /*operationResult*/,
                ScrollPresenterViewChangeResult::Ignored     /*unused priorNonAnimatedOperationsResult*/,
                ScrollPresenterViewChangeResult::Ignored     /*unused priorAnimatedOperationsResult*/,
                true  /*completeNonAnimatedOperation*/,
                true  /*completeAnimatedOperation*/,
                false /*completePriorNonAnimatedOperations*/,
                false /*completePriorAnimatedOperations*/);
        }

        if (m_interactionTracker)
        {
            if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
            {
                UpdatePositionBoundaries(nullptr);
            }
            if (m_translationExpressionAnimation && m_zoomFactorExpressionAnimation)
            {
                StopTransformExpressionAnimations(oldContent);
            }
            ScrollToOffsets(0.0 /*zoomedHorizontalOffset*/, 0.0 /*zoomedVerticalOffset*/);
        }
    }
}

void ScrollPresenter::UpdatePositionBoundaries(
    const winrt::UIElement& content)
{
    MUX_ASSERT(m_minPositionExpressionAnimation);
    MUX_ASSERT(m_maxPositionExpressionAnimation);
    MUX_ASSERT(m_interactionTracker);

    if (!content)
    {
        const winrt::float3 boundaryPosition(0.0f);

        m_interactionTracker.MinPosition(boundaryPosition);
        m_interactionTracker.MaxPosition(boundaryPosition);
    }
    else
    {
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"contentSizeX", m_unzoomedExtentWidth);
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"contentSizeY", m_unzoomedExtentHeight);
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"contentLayoutOffsetX", m_contentLayoutOffsetX);
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"contentLayoutOffsetY", m_contentLayoutOffsetY);

        m_minPositionExpressionAnimation.SetScalarParameter(L"contentSizeX", static_cast<float>(m_unzoomedExtentWidth));
        m_maxPositionExpressionAnimation.SetScalarParameter(L"contentSizeX", static_cast<float>(m_unzoomedExtentWidth));
        m_minPositionExpressionAnimation.SetScalarParameter(L"contentSizeY", static_cast<float>(m_unzoomedExtentHeight));
        m_maxPositionExpressionAnimation.SetScalarParameter(L"contentSizeY", static_cast<float>(m_unzoomedExtentHeight));

        m_minPositionExpressionAnimation.SetScalarParameter(L"contentLayoutOffsetX", m_contentLayoutOffsetX);
        m_maxPositionExpressionAnimation.SetScalarParameter(L"contentLayoutOffsetX", m_contentLayoutOffsetX);
        m_minPositionExpressionAnimation.SetScalarParameter(L"contentLayoutOffsetY", m_contentLayoutOffsetY);
        m_maxPositionExpressionAnimation.SetScalarParameter(L"contentLayoutOffsetY", m_contentLayoutOffsetY);

        m_interactionTracker.StartAnimation(s_minPositionSourcePropertyName, m_minPositionExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_minPositionSourcePropertyName /*propertyName*/);

        m_interactionTracker.StartAnimation(s_maxPositionSourcePropertyName, m_maxPositionExpressionAnimation);
        RaiseExpressionAnimationStatusChanged(true /*isExpressionAnimationStarted*/, s_maxPositionSourcePropertyName /*propertyName*/);
    }

#ifdef DBG
    DumpMinMaxPositions();
#endif // DBG
}

void ScrollPresenter::UpdateTransformSource(
    const winrt::UIElement& oldContent,
    const winrt::UIElement& newContent)
{
    MUX_ASSERT(m_translationExpressionAnimation && m_zoomFactorExpressionAnimation);
    MUX_ASSERT(m_interactionTracker);

    StopTransformExpressionAnimations(oldContent);
    StartTransformExpressionAnimations(newContent);
}

void ScrollPresenter::UpdateState(
    const winrt::ScrollingInteractionState& state)
{
    if (state != winrt::ScrollingInteractionState::Idle)
    {
        // Restart the interrupted expression animations sooner than planned to visualize the new view change immediately.
        StartTranslationAndZoomFactorExpressionAnimations(true /*interruptCountdown*/);
    }

    if (state != m_state)
    {
        m_state = state;
        RaiseStateChanged();
    }
}

void ScrollPresenter::UpdateExpressionAnimationSources()
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(m_expressionAnimationSources);

    m_expressionAnimationSources.InsertVector2(s_extentSourcePropertyName, { static_cast<float>(m_unzoomedExtentWidth), static_cast<float>(m_unzoomedExtentHeight) });
    m_expressionAnimationSources.InsertVector2(s_viewportSourcePropertyName, { static_cast<float>(m_viewportWidth), static_cast<float>(m_viewportHeight) });
}

void ScrollPresenter::UpdateUnzoomedExtentAndViewport(
    bool renderSizeChanged,
    double unzoomedExtentWidth,
    double unzoomedExtentHeight,
    double viewportWidth,
    double viewportHeight)
{
    const winrt::UIElement content = Content();
    const winrt::UIElement thisAsUIE = *this;
    const double oldUnzoomedExtentWidth = m_unzoomedExtentWidth;
    const double oldUnzoomedExtentHeight = m_unzoomedExtentHeight;
    const double oldViewportWidth = m_viewportWidth;
    const double oldViewportHeight = m_viewportHeight;

    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"renderSizeChanged", renderSizeChanged);
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL_DBL, METH_NAME, this, L"old/new unzoomedExtentWidth", oldUnzoomedExtentWidth, unzoomedExtentWidth);
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL_DBL, METH_NAME, this, L"old/new unzoomedExtentHeight", oldUnzoomedExtentHeight, unzoomedExtentHeight);
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL_DBL, METH_NAME, this, L"old/new viewportWidth", oldViewportWidth, viewportWidth);
    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL_DBL, METH_NAME, this, L"old/new viewportHeight", oldViewportHeight, viewportHeight);

    MUX_ASSERT(!isinf(unzoomedExtentWidth));
    MUX_ASSERT(!isnan(unzoomedExtentWidth));
    MUX_ASSERT(!isinf(unzoomedExtentHeight));
    MUX_ASSERT(!isnan(unzoomedExtentHeight));

    MUX_ASSERT(!isinf(viewportWidth));
    MUX_ASSERT(!isnan(viewportWidth));
    MUX_ASSERT(!isinf(viewportHeight));
    MUX_ASSERT(!isnan(viewportHeight));

    MUX_ASSERT(unzoomedExtentWidth >= 0.0);
    MUX_ASSERT(unzoomedExtentHeight >= 0.0);
    MUX_ASSERT(!(!content && unzoomedExtentWidth != 0.0));
    MUX_ASSERT(!(!content && unzoomedExtentHeight != 0.0));

    const bool horizontalExtentChanged = oldUnzoomedExtentWidth != unzoomedExtentWidth;
    const bool verticalExtentChanged = oldUnzoomedExtentHeight != unzoomedExtentHeight;
    const bool extentChanged = horizontalExtentChanged || verticalExtentChanged;

    const bool horizontalViewportChanged = oldViewportWidth != viewportWidth;
    const bool verticalViewportChanged = oldViewportHeight != viewportHeight;
    const bool viewportChanged = horizontalViewportChanged || verticalViewportChanged;

    m_unzoomedExtentWidth = unzoomedExtentWidth;
    m_unzoomedExtentHeight = unzoomedExtentHeight;

    m_viewportWidth = viewportWidth;
    m_viewportHeight = viewportHeight;

    if (m_expressionAnimationSources)
    {
        UpdateExpressionAnimationSources();
    }

    if ((extentChanged || renderSizeChanged) && content)
    {
        OnContentSizeChanged(content);
    }

    if (extentChanged || viewportChanged)
    {
        MaximizeInteractionTrackerOperationsTicksCountdown();
        UpdateScrollAutomationPatternProperties();
    }

    if (horizontalExtentChanged || horizontalViewportChanged)
    {
        // Updating the horizontal scroll mode because GetComputedScrollMode is function of the scrollable width.
        UpdateVisualInteractionSourceMode(ScrollPresenterDimension::HorizontalScroll);

        UpdateScrollControllerValues(ScrollPresenterDimension::HorizontalScroll);
    }

    if (verticalExtentChanged || verticalViewportChanged)
    {
        // Updating the vertical scroll mode because GetComputedScrollMode is function of the scrollable height.
        UpdateVisualInteractionSourceMode(ScrollPresenterDimension::VerticalScroll);

        UpdateScrollControllerValues(ScrollPresenterDimension::VerticalScroll);
    }

    if (horizontalViewportChanged && m_horizontalSnapPoints && m_horizontalSnapPointsNeedViewportUpdates)
    {
        // At least one horizontal scroll snap point is not near-aligned and is thus sensitive to the
        // viewport width. Regenerate and set up all horizontal scroll snap points.
        const auto horizontalSnapPoints = m_horizontalSnapPoints.try_as<winrt::IObservableVector<winrt::ScrollSnapPointBase>>();
        const bool horizontalSnapPointsNeedViewportUpdates = SnapPointsViewportChangedHelper(
            horizontalSnapPoints,
            m_viewportWidth);
        MUX_ASSERT(horizontalSnapPointsNeedViewportUpdates);

        RegenerateSnapPointsSet(horizontalSnapPoints, &m_sortedConsolidatedHorizontalSnapPoints);
        SetupSnapPoints(&m_sortedConsolidatedHorizontalSnapPoints, ScrollPresenterDimension::HorizontalScroll);
    }

    if (verticalViewportChanged && m_verticalSnapPoints && m_verticalSnapPointsNeedViewportUpdates)
    {
        // At least one vertical scroll snap point is not near-aligned and is thus sensitive to the
        // viewport height. Regenerate and set up all vertical scroll snap points.
        const auto verticalSnapPoints = m_verticalSnapPoints.try_as<winrt::IObservableVector<winrt::ScrollSnapPointBase>>();
        const bool verticalSnapPointsNeedViewportUpdates = SnapPointsViewportChangedHelper(
            verticalSnapPoints,
            m_viewportHeight);
        MUX_ASSERT(verticalSnapPointsNeedViewportUpdates);

        RegenerateSnapPointsSet(verticalSnapPoints, &m_sortedConsolidatedVerticalSnapPoints);
        SetupSnapPoints(&m_sortedConsolidatedVerticalSnapPoints, ScrollPresenterDimension::VerticalScroll);
    }

    if (extentChanged)
    {
        RaiseExtentChanged();
    }
}

// Raise automation peer property change events
void ScrollPresenter::UpdateScrollAutomationPatternProperties()
{
    if (winrt::AutomationPeer automationPeer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        winrt::ScrollPresenterAutomationPeer scrollPresenterAutomationPeer = automationPeer.try_as<winrt::ScrollPresenterAutomationPeer>();
        if (scrollPresenterAutomationPeer)
        {
            winrt::get_self<ScrollPresenterAutomationPeer>(scrollPresenterAutomationPeer)->UpdateScrollPatternProperties();
        }
    }
}

void ScrollPresenter::UpdateAnticipatedOffset(ScrollPresenterDimension dimension, double zoomedOffset)
{
    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        if (m_anticipatedZoomedHorizontalOffset != zoomedOffset)
        {
            SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL_DBL, METH_NAME, this, L"old/new anticipatedZoomedHorizontalOffset", m_anticipatedZoomedHorizontalOffset, zoomedOffset);

            m_anticipatedZoomedHorizontalOffset = zoomedOffset;
        }
    }
    else
    {
        MUX_ASSERT(dimension == ScrollPresenterDimension::VerticalScroll);
        if (m_anticipatedZoomedVerticalOffset != zoomedOffset)
        {
            SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL_DBL, METH_NAME, this, L"old/new anticipatedZoomedVerticalOffset", m_anticipatedZoomedVerticalOffset, zoomedOffset);

            m_anticipatedZoomedVerticalOffset = zoomedOffset;
        }
    }
}

void ScrollPresenter::UpdateAnticipatedZoomFactor(float zoomFactor)
{
    if (m_anticipatedZoomFactor != zoomFactor)
    {
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"old/new anticipatedZoomFactor", m_anticipatedZoomFactor, zoomFactor);

        m_anticipatedZoomFactor = zoomFactor;
    }
}

void ScrollPresenter::UpdateOffset(ScrollPresenterDimension dimension, double zoomedOffset)
{
    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        if (m_zoomedHorizontalOffset != zoomedOffset)
        {
            SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL_DBL, METH_NAME, this, L"old/new zoomedHorizontalOffset", m_zoomedHorizontalOffset, zoomedOffset);

            m_zoomedHorizontalOffset = zoomedOffset;
        }
    }
    else
    {
        MUX_ASSERT(dimension == ScrollPresenterDimension::VerticalScroll);
        if (m_zoomedVerticalOffset != zoomedOffset)
        {
            SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL_DBL, METH_NAME, this, L"old/new zoomedVerticalOffset", m_zoomedVerticalOffset, zoomedOffset);

            m_zoomedVerticalOffset = zoomedOffset;
        }
    }
}

void ScrollPresenter::UpdateScrollControllerIsScrollable(ScrollPresenterDimension dimension)
{
    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        if (m_horizontalScrollController)
        {
            m_horizontalScrollController.get().SetIsScrollable(ComputedHorizontalScrollMode() == winrt::ScrollingScrollMode::Enabled);
        }
    }
    else
    {
        MUX_ASSERT(dimension == ScrollPresenterDimension::VerticalScroll);

        if (m_verticalScrollController)
        {
            m_verticalScrollController.get().SetIsScrollable(ComputedVerticalScrollMode() == winrt::ScrollingScrollMode::Enabled);
        }
    }
}

void ScrollPresenter::UpdateScrollControllerValues(ScrollPresenterDimension dimension)
{
    // To avoid rounding imprecisions incorrectly causing a scroll controller to be declared scrollable,
    // no scrollable size smaller than this epsilon is provided to it.
    const double c_scrollableEpsilon{ 0.0001 };

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        double scrollableWidth = ScrollableWidth();

        if (scrollableWidth < c_scrollableEpsilon)
        {
            scrollableWidth = 0.0;
        }

        if (m_horizontalScrollController)
        {
            m_horizontalScrollController.get().SetValues(
                0.0 /*minOffset*/,
                scrollableWidth,
                std::min(scrollableWidth, m_zoomedHorizontalOffset) /*offset*/,
                ViewportWidth() /*viewportLength*/);
        }
    }
    else
    {
        MUX_ASSERT(dimension == ScrollPresenterDimension::VerticalScroll);

        if (m_verticalScrollController)
        {
            double scrollableHeight = ScrollableHeight();

            if (scrollableHeight < c_scrollableEpsilon)
            {
                scrollableHeight = 0.0;
            }

            m_verticalScrollController.get().SetValues(
                0.0 /*minOffset*/,
                scrollableHeight /*maxOffset*/,
                std::min(scrollableHeight, m_zoomedVerticalOffset) /*offset*/,
                ViewportHeight() /*viewportLength*/);
        }
    }
}

void ScrollPresenter::UpdateVisualInteractionSourceMode(ScrollPresenterDimension dimension)
{
    const winrt::ScrollingScrollMode scrollMode = GetComputedScrollMode(dimension);

    if (m_scrollPresenterVisualInteractionSource)
    {
        SetupVisualInteractionSourceMode(
            m_scrollPresenterVisualInteractionSource,
            dimension,
            scrollMode);

#ifdef IsMouseWheelScrollDisabled
        SetupVisualInteractionSourcePointerWheelConfig(
            m_scrollPresenterVisualInteractionSource,
            dimension,
            GetComputedMouseWheelScrollMode(dimension));
#endif
    }

    UpdateScrollControllerIsScrollable(dimension);
}

void ScrollPresenter::UpdateManipulationRedirectionMode()
{
    if (m_scrollPresenterVisualInteractionSource)
    {
        SetupVisualInteractionSourceRedirectionMode(m_scrollPresenterVisualInteractionSource);
    }
}

void ScrollPresenter::OnContentSizeChanged(const winrt::UIElement& content)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
    {
        UpdatePositionBoundaries(content);
    }

    if (m_interactionTracker && m_translationExpressionAnimation && m_zoomFactorExpressionAnimation)
    {
        SetupTransformExpressionAnimations(content);
    }
}

void ScrollPresenter::OnViewChanged(bool horizontalOffsetChanged, bool verticalOffsetChanged)
{
    //SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, horizontalOffsetChanged, verticalOffsetChanged);
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_DBL_FLT, METH_NAME, this, m_zoomedHorizontalOffset, m_zoomedVerticalOffset, m_zoomFactor);

    if (horizontalOffsetChanged)
    {
        UpdateScrollControllerValues(ScrollPresenterDimension::HorizontalScroll);
    }

    if (verticalOffsetChanged)
    {
        UpdateScrollControllerValues(ScrollPresenterDimension::VerticalScroll);
    }

    UpdateScrollAutomationPatternProperties();

    RaiseViewChanged();
}

void ScrollPresenter::OnContentLayoutOffsetChanged(ScrollPresenterDimension dimension)
{
#ifdef DBG
    MUX_ASSERT(dimension == ScrollPresenterDimension::HorizontalScroll || dimension == ScrollPresenterDimension::VerticalScroll);

    if (dimension == ScrollPresenterDimension::HorizontalScroll)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"Horizontal", m_contentLayoutOffsetX);
    }
    else
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"Vertical", m_contentLayoutOffsetY);
    }
#endif

    com_ptr<ScrollPresenterTestHooks> globalTestHooks = ScrollPresenterTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        if (dimension == ScrollPresenterDimension::HorizontalScroll)
        {
            globalTestHooks->NotifyContentLayoutOffsetXChanged(*this);
        }
        else
        {
            globalTestHooks->NotifyContentLayoutOffsetYChanged(*this);
        }
    }

    if (m_minPositionExpressionAnimation && m_maxPositionExpressionAnimation)
    {
        const winrt::UIElement content = Content();

        if (content)
        {
            UpdatePositionBoundaries(content);
        }
    }

    if (m_expressionAnimationSources)
    {
        m_expressionAnimationSources.InsertVector2(s_offsetSourcePropertyName, { m_contentLayoutOffsetX, m_contentLayoutOffsetY });
    }

    if (m_scrollPresenterVisualInteractionSource)
    {
        SetupVisualInteractionSourceCenterPointModifier(
            m_scrollPresenterVisualInteractionSource,
            dimension,
            false /*flowDirectionChanged*/);
    }
}

void ScrollPresenter::ChangeOffsetsPrivate(
    double zoomedHorizontalOffset,
    double zoomedVerticalOffset,
    ScrollPresenterViewKind offsetsKind,
    winrt::ScrollingScrollOptions const& options,
    winrt::BringIntoViewRequestedEventArgs const& bringIntoViewRequestedEventArgs,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    int32_t existingViewChangeCorrelationId,
    _Out_opt_ int32_t* viewChangeCorrelationId)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        zoomedHorizontalOffset,
        zoomedVerticalOffset,
        TypeLogging::ScrollOptionsToString(options).c_str());
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str(),
        TypeLogging::ScrollPresenterViewKindToString(offsetsKind).c_str());
#ifdef DBG
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        L"existingViewChangeCorrelationId",
        existingViewChangeCorrelationId);

    if (bringIntoViewRequestedEventArgs)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, L"%s[0x%p](bringIntoViewRequestedEventArgs: AnimationDesired:%d, H/V AlignmentRatio:%lf,%lf, H/V Offset:%f,%f, TargetRect:%s, TargetElement:0x%p)\n",
            METH_NAME, this,
            bringIntoViewRequestedEventArgs.AnimationDesired(),
            bringIntoViewRequestedEventArgs.HorizontalAlignmentRatio(), bringIntoViewRequestedEventArgs.VerticalAlignmentRatio(),
            bringIntoViewRequestedEventArgs.HorizontalOffset(), bringIntoViewRequestedEventArgs.VerticalOffset(),
            TypeLogging::RectToString(bringIntoViewRequestedEventArgs.TargetRect()).c_str(),
            bringIntoViewRequestedEventArgs.TargetElement());
    }
#endif

    if (viewChangeCorrelationId)
    {
        *viewChangeCorrelationId = s_noOpCorrelationId;
    }

    winrt::ScrollingAnimationMode animationMode = options ? options.AnimationMode() : ScrollingScrollOptions::s_defaultAnimationMode;
    winrt::ScrollingSnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollingScrollOptions::s_defaultSnapPointsMode;
    InteractionTrackerAsyncOperationType operationType;

    animationMode = GetComputedAnimationMode(animationMode);

    switch (animationMode)
    {
    case winrt::ScrollingAnimationMode::Disabled:
    {
        switch (offsetsKind)
        {
        case ScrollPresenterViewKind::Absolute:
#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
        case ScrollPresenterViewKind::RelativeToEndOfInertiaView:
#endif
        {
            operationType = InteractionTrackerAsyncOperationType::TryUpdatePosition;
            break;
        }
        case ScrollPresenterViewKind::RelativeToCurrentView:
        {
            operationType = InteractionTrackerAsyncOperationType::TryUpdatePositionBy;
            break;
        }
        }
        break;
    }
    case winrt::ScrollingAnimationMode::Enabled:
    {
        switch (offsetsKind)
        {
        case ScrollPresenterViewKind::Absolute:
        case ScrollPresenterViewKind::RelativeToCurrentView:
#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
        case ScrollPresenterViewKind::RelativeToEndOfInertiaView:
#endif
        {
            operationType = InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation;
            break;
        }
        }
        break;
    }
    }

    if (!Content())
    {
        // When there is no content, skip the view change request and return -1, indicating that no action was taken.
        return;
    }

    // When the ScrollPresenter is not loaded or not set up yet, delay the offsets change request until it gets loaded.
    // OnCompositionTargetRendering will launch the delayed changes at that point.
    bool delayOperation = !IsLoadedAndSetUp();

    com_ptr<ScrollingScrollOptions> optionsClone{ nullptr };

    // Clone the options for this request if needed. The clone or original options will be used if the operation ever gets processed.
    const bool isScrollControllerRequest =
        static_cast<char>(operationTrigger) &
        (static_cast<char>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) |
            static_cast<char>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest));

    if (options && !isScrollControllerRequest)
    {
        // Options are cloned so that they can be modified by the caller after this offsets change call without affecting the outcome of the operation.
        optionsClone = winrt::make_self<ScrollingScrollOptions>(
            animationMode,
            snapPointsMode);
    }

    if (!delayOperation)
    {
        MUX_ASSERT(m_interactionTracker);

        // Prevent any existing delayed operation from being processed after this request and overriding it.
        // All delayed operations are completed with the Interrupted result.
        CompleteDelayedOperations();

        HookCompositionTargetRendering();
    }

    std::shared_ptr<ViewChange> offsetsChange = nullptr;

    if (static_cast<char>(operationTrigger) & static_cast<char>(InteractionTrackerAsyncOperationTrigger::BringIntoViewRequest))
    {
        // Bring-into-view operations use a richer version of OffsetsChange which includes
        // information extracted from the BringIntoViewRequestedEventArgs instance.
        // This allows to use ComputeBringIntoViewUpdatedTargetOffsets just before invoking
        // the InteractionTracker's TryUpdatePosition.
        offsetsChange = std::make_shared<BringIntoViewOffsetsChange>(
            this,
            zoomedHorizontalOffset,
            zoomedVerticalOffset,
            offsetsKind,
            optionsClone ? static_cast<winrt::IInspectable>(*optionsClone) : static_cast<winrt::IInspectable>(options),
            bringIntoViewRequestedEventArgs.TargetElement(),
            bringIntoViewRequestedEventArgs.TargetRect(),
            bringIntoViewRequestedEventArgs.HorizontalAlignmentRatio(),
            bringIntoViewRequestedEventArgs.VerticalAlignmentRatio(),
            bringIntoViewRequestedEventArgs.HorizontalOffset(),
            bringIntoViewRequestedEventArgs.VerticalOffset());
    }
    else
    {
        offsetsChange = std::make_shared<OffsetsChange>(
            zoomedHorizontalOffset,
            zoomedVerticalOffset,
            offsetsKind,
            optionsClone ? static_cast<winrt::IInspectable>(*optionsClone) : static_cast<winrt::IInspectable>(options));
    }

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation(
        std::make_shared<InteractionTrackerAsyncOperation>(
            operationType,
            operationTrigger,
            delayOperation,
            offsetsChange));

    if (operationTrigger != InteractionTrackerAsyncOperationTrigger::DirectViewChange)
    {
        // User-triggered or bring-into-view operations are processed as quickly as possible by minimizing their TicksCountDown
        const int ticksCountdown = GetInteractionTrackerOperationsTicksCountdown();

        interactionTrackerAsyncOperation->SetTicksCountdown(std::max(1, ticksCountdown));
    }

    m_interactionTrackerAsyncOperations.push_back(interactionTrackerAsyncOperation);

    if (viewChangeCorrelationId)
    {
        if (existingViewChangeCorrelationId != s_noOpCorrelationId)
        {
            interactionTrackerAsyncOperation->SetViewChangeCorrelationId(existingViewChangeCorrelationId);
            *viewChangeCorrelationId = existingViewChangeCorrelationId;
        }
        else
        {
            m_latestViewChangeCorrelationId = GetNextViewChangeCorrelationId();
            interactionTrackerAsyncOperation->SetViewChangeCorrelationId(m_latestViewChangeCorrelationId);
            *viewChangeCorrelationId = m_latestViewChangeCorrelationId;
        }
    }
    else if (existingViewChangeCorrelationId != s_noOpCorrelationId)
    {
        interactionTrackerAsyncOperation->SetViewChangeCorrelationId(existingViewChangeCorrelationId);
    }
}

void ScrollPresenter::ChangeOffsetsWithAdditionalVelocityPrivate(
    winrt::float2 offsetsVelocity,
    winrt::float2 anticipatedOffsetsChange,
    winrt::IReference<winrt::float2> inertiaDecayRate,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    _Out_opt_ int32_t* viewChangeCorrelationId)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_STR, METH_NAME, this,
        TypeLogging::Float2ToString(offsetsVelocity).c_str(),
        TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str(),
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str());

    if (viewChangeCorrelationId)
    {
        *viewChangeCorrelationId = s_noOpCorrelationId;
    }

    if (!Content())
    {
        // When there is no content, skip the view change request and return -1, indicating that no action was taken.
        return;
    }

    // When the ScrollPresenter is not loaded or not set up yet, delay the offsets change request until it gets loaded.
    // OnCompositionTargetRendering will launch the delayed changes at that point.
    bool delayOperation = !IsLoadedAndSetUp();

    std::shared_ptr<ViewChangeBase> offsetsChangeWithAdditionalVelocity =
        std::make_shared<OffsetsChangeWithAdditionalVelocity>(
            offsetsVelocity, anticipatedOffsetsChange, inertiaDecayRate);

    if (!delayOperation)
    {
        MUX_ASSERT(m_interactionTracker);

        // Prevent any existing delayed operation from being processed after this request and overriding it.
        // All delayed operations are completed with the Interrupted result.
        CompleteDelayedOperations();

        HookCompositionTargetRendering();
    }

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation(
        std::make_shared<InteractionTrackerAsyncOperation>(
            InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity,
            operationTrigger,
            delayOperation,
            offsetsChangeWithAdditionalVelocity));

    if (operationTrigger != InteractionTrackerAsyncOperationTrigger::DirectViewChange)
    {
        // User-triggered or bring-into-view operations are processed as quickly as possible by minimizing their TicksCountDown
        const int ticksCountdown = GetInteractionTrackerOperationsTicksCountdown();

        interactionTrackerAsyncOperation->SetTicksCountdown(std::max(1, ticksCountdown));
    }

    m_interactionTrackerAsyncOperations.push_back(interactionTrackerAsyncOperation);

    if (viewChangeCorrelationId)
    {
        m_latestViewChangeCorrelationId = GetNextViewChangeCorrelationId();
        interactionTrackerAsyncOperation->SetViewChangeCorrelationId(m_latestViewChangeCorrelationId);
        *viewChangeCorrelationId = m_latestViewChangeCorrelationId;
    }
}

void ScrollPresenter::ChangeZoomFactorPrivate(
    float zoomFactor,
    winrt::IReference<winrt::float2> centerPoint,
    ScrollPresenterViewKind zoomFactorKind,
    winrt::ScrollingZoomOptions const& options,
    _Out_opt_ int32_t* viewChangeCorrelationId)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        zoomFactor);
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::ScrollPresenterViewKindToString(zoomFactorKind).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str());

    if (viewChangeCorrelationId)
    {
        *viewChangeCorrelationId = s_noOpCorrelationId;
    }

    if (!Content())
    {
        // When there is no content, skip the view change request and return -1, indicating that no action was taken.
        return;
    }

    winrt::ScrollingAnimationMode animationMode = options ? options.AnimationMode() : ScrollingZoomOptions::s_defaultAnimationMode;
    winrt::ScrollingSnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollingZoomOptions::s_defaultSnapPointsMode;
    InteractionTrackerAsyncOperationType operationType;

    animationMode = GetComputedAnimationMode(animationMode);

    switch (animationMode)
    {
    case winrt::ScrollingAnimationMode::Disabled:
    {
        switch (zoomFactorKind)
        {
        case ScrollPresenterViewKind::Absolute:
#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
        case ScrollPresenterViewKind::RelativeToEndOfInertiaView:
#endif
        case ScrollPresenterViewKind::RelativeToCurrentView:
        {
            operationType = InteractionTrackerAsyncOperationType::TryUpdateScale;
            break;
        }
        }
        break;
    }
    case winrt::ScrollingAnimationMode::Enabled:
    {
        switch (zoomFactorKind)
        {
        case ScrollPresenterViewKind::Absolute:
#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
        case ScrollPresenterViewKind::RelativeToEndOfInertiaView:
#endif
        case ScrollPresenterViewKind::RelativeToCurrentView:
        {
            operationType = InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation;
            break;
        }
        }
        break;
    }
    }

    // When the ScrollPresenter is not loaded or not set up yet (delayOperation==True), delay the zoomFactor change request until it gets loaded.
    // OnCompositionTargetRendering will launch the delayed changes at that point.
    bool delayOperation = !IsLoadedAndSetUp();

    // Set to True when workaround for RS5 InteractionTracker bug 18827625 was applied (i.e. on-going TryUpdateScaleWithAnimation operation
    // is interrupted with TryUpdateScale operation).
    bool scaleChangeWithAnimationInterrupted = false;

    com_ptr<ScrollingZoomOptions> optionsClone{ nullptr };

    // Clone the original options if any. The clone will be used if the operation ever gets processed.
    if (options)
    {
        // Options are cloned so that they can be modified by the caller after this zoom factor change call without affecting the outcome of the operation.
        optionsClone = winrt::make_self<ScrollingZoomOptions>(
            animationMode,
            snapPointsMode);
    }

    if (!delayOperation)
    {
        MUX_ASSERT(m_interactionTracker);

        // Prevent any existing delayed operation from being processed after this request and overriding it.
        // All delayed operations are completed with the Interrupted result.
        CompleteDelayedOperations();

        HookCompositionTargetRendering();
    }

    std::shared_ptr<ViewChange> zoomFactorChange =
        std::make_shared<ZoomFactorChange>(
            zoomFactor,
            centerPoint,
            zoomFactorKind,
            optionsClone ? static_cast<winrt::IInspectable>(*optionsClone) : static_cast<winrt::IInspectable>(options));

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation(
        std::make_shared<InteractionTrackerAsyncOperation>(
            operationType,
            InteractionTrackerAsyncOperationTrigger::DirectViewChange,
            delayOperation,
            zoomFactorChange));

    m_interactionTrackerAsyncOperations.push_back(interactionTrackerAsyncOperation);

    // Workaround for InteractionTracker bug 22414894 - calling TryUpdateScale after a non-animated view change during the same tick results in an incorrect position.
    // That non-animated view change needs to complete before this TryUpdateScale gets invoked.
    interactionTrackerAsyncOperation->SetRequiredOperation(GetLastNonAnimatedInteractionTrackerOperation(interactionTrackerAsyncOperation));

    if (viewChangeCorrelationId)
    {
        m_latestViewChangeCorrelationId = GetNextViewChangeCorrelationId();
        interactionTrackerAsyncOperation->SetViewChangeCorrelationId(m_latestViewChangeCorrelationId);
        *viewChangeCorrelationId = m_latestViewChangeCorrelationId;
    }
}

void ScrollPresenter::ChangeZoomFactorWithAdditionalVelocityPrivate(
    float zoomFactorVelocity,
    float anticipatedZoomFactorChange,
    winrt::IReference<winrt::float2> centerPoint,
    winrt::IReference<float> inertiaDecayRate,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    _Out_opt_ int32_t* viewChangeCorrelationId)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this,
        zoomFactorVelocity,
        anticipatedZoomFactorChange);
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR_STR, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(),
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str());

    if (viewChangeCorrelationId)
    {
        *viewChangeCorrelationId = s_noOpCorrelationId;
    }

    if (!Content())
    {
        // When there is no content, skip the view change request and return -1, indicating that no action was taken.
        return;
    }

    // When the ScrollPresenter is not loaded or not set up yet (delayOperation==True), delay the zoom factor change request until it gets loaded.
    // OnCompositionTargetRendering will launch the delayed changes at that point.
    bool delayOperation = !IsLoadedAndSetUp();

    std::shared_ptr<ViewChangeBase> zoomFactorChangeWithAdditionalVelocity =
        std::make_shared<ZoomFactorChangeWithAdditionalVelocity>(
            zoomFactorVelocity, anticipatedZoomFactorChange, centerPoint, inertiaDecayRate);

    if (!delayOperation)
    {
        MUX_ASSERT(m_interactionTracker);

        // Prevent any existing delayed operation from being processed after this request and overriding it.
        // All delayed operations are completed with the Interrupted result.
        CompleteDelayedOperations();

        HookCompositionTargetRendering();
    }

    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation(
        std::make_shared<InteractionTrackerAsyncOperation>(
            InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity,
            operationTrigger,
            delayOperation,
            zoomFactorChangeWithAdditionalVelocity));

    if (operationTrigger != InteractionTrackerAsyncOperationTrigger::DirectViewChange)
    {
        // User-triggered operations are processed as quickly as possible by minimizing their TicksCountDown
        const int ticksCountdown = GetInteractionTrackerOperationsTicksCountdown();

        interactionTrackerAsyncOperation->SetTicksCountdown(std::max(1, ticksCountdown));
    }

    m_interactionTrackerAsyncOperations.push_back(interactionTrackerAsyncOperation);

    if (viewChangeCorrelationId)
    {
        m_latestViewChangeCorrelationId = GetNextViewChangeCorrelationId();
        interactionTrackerAsyncOperation->SetViewChangeCorrelationId(m_latestViewChangeCorrelationId);
        *viewChangeCorrelationId = m_latestViewChangeCorrelationId;
    }
}

void ScrollPresenter::ProcessDequeuedViewChange(std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, interactionTrackerAsyncOperation);

    MUX_ASSERT(IsLoadedAndSetUp());
    MUX_ASSERT(!interactionTrackerAsyncOperation->IsQueued());

    std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();

    MUX_ASSERT(viewChangeBase);

    switch (interactionTrackerAsyncOperation->GetOperationType())
    {
        case InteractionTrackerAsyncOperationType::TryUpdatePosition:
        case InteractionTrackerAsyncOperationType::TryUpdatePositionBy:
        case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation:
        {
            std::shared_ptr<OffsetsChange> offsetsChange = std::reinterpret_pointer_cast<OffsetsChange>(viewChangeBase);

            ProcessOffsetsChange(
                interactionTrackerAsyncOperation->GetOperationTrigger() /*operationTrigger*/,
                offsetsChange,
                interactionTrackerAsyncOperation->GetViewChangeCorrelationId() /*offsetsChangeId*/,
                true /*isForAsyncOperation*/);
            break;
        }
        case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
        {
            std::shared_ptr<OffsetsChangeWithAdditionalVelocity> offsetsChangeWithAdditionalVelocity = std::reinterpret_pointer_cast<OffsetsChangeWithAdditionalVelocity>(viewChangeBase);

            ProcessOffsetsChange(
                interactionTrackerAsyncOperation->GetOperationTrigger() /*operationTrigger*/,
                offsetsChangeWithAdditionalVelocity);
            break;
        }
        case InteractionTrackerAsyncOperationType::TryUpdateScale:
        case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation:
        {
            std::shared_ptr<ZoomFactorChange> zoomFactorChange = std::reinterpret_pointer_cast<ZoomFactorChange>(viewChangeBase);

            ProcessZoomFactorChange(
                zoomFactorChange,
                interactionTrackerAsyncOperation->GetViewChangeCorrelationId() /*zoomFactorChangeId*/);
            break;
        }
        case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity:
        {
            std::shared_ptr<ZoomFactorChangeWithAdditionalVelocity> zoomFactorChangeWithAdditionalVelocity = std::reinterpret_pointer_cast<ZoomFactorChangeWithAdditionalVelocity>(viewChangeBase);

            ProcessZoomFactorChange(
                interactionTrackerAsyncOperation->GetOperationTrigger() /*operationTrigger*/,
                zoomFactorChangeWithAdditionalVelocity);
            break;
        }
        default:
        {
            MUX_ASSERT(false);
            break;
        }
    }
    interactionTrackerAsyncOperation->SetRequestId(m_latestInteractionTrackerRequest);
}

// Launches an InteractionTracker request to change the offsets.
void ScrollPresenter::ProcessOffsetsChange(
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    std::shared_ptr<OffsetsChange> offsetsChange,
    int32_t offsetsChangeCorrelationId,
    bool isForAsyncOperation)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(offsetsChange);

    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        L"operationTrigger", TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str());
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        L"viewKind", TypeLogging::ScrollPresenterViewKindToString(offsetsChange->ViewKind()).c_str());
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        L"offsetsChangeCorrelationId", offsetsChangeCorrelationId);
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        L"isForAsyncOperation", isForAsyncOperation);

    double zoomedHorizontalOffset = offsetsChange->ZoomedHorizontalOffset();
    double zoomedVerticalOffset = offsetsChange->ZoomedVerticalOffset();
    winrt::ScrollingScrollOptions options = offsetsChange->Options().try_as<winrt::ScrollingScrollOptions>();

    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this,
        L"zoomedHorizontalOffset",
        zoomedHorizontalOffset);
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this,
        L"zoomedVerticalOffset",
        zoomedVerticalOffset);
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        L"options",
        TypeLogging::ScrollOptionsToString(options).c_str());

    winrt::ScrollingAnimationMode animationMode = options ? options.AnimationMode() : ScrollingScrollOptions::s_defaultAnimationMode;
    const winrt::ScrollingSnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollingScrollOptions::s_defaultSnapPointsMode;

    animationMode = GetComputedAnimationMode(animationMode);

    if (static_cast<char>(operationTrigger) & static_cast<char>(InteractionTrackerAsyncOperationTrigger::BringIntoViewRequest))
    {
        if (winrt::UIElement content = Content())
        {
            std::shared_ptr<BringIntoViewOffsetsChange> bringIntoViewOffsetsChange = std::reinterpret_pointer_cast<BringIntoViewOffsetsChange>(offsetsChange);

            if (bringIntoViewOffsetsChange)
            {
                // The target Element may have moved within the Content since the bring-into-view operation was
                // initiated one or more ticks ago in ScrollPresenter::OnBringIntoViewRequestedHandler.
                // The target offsets are therefore re-evaluated according to the latest Element position and size.
                ComputeBringIntoViewUpdatedTargetOffsets(
                    content,
                    bringIntoViewOffsetsChange->Element(),
                    bringIntoViewOffsetsChange->ElementRect(),
                    snapPointsMode,
                    bringIntoViewOffsetsChange->HorizontalAlignmentRatio(),
                    bringIntoViewOffsetsChange->VerticalAlignmentRatio(),
                    bringIntoViewOffsetsChange->HorizontalOffset(),
                    bringIntoViewOffsetsChange->VerticalOffset(),
                    &zoomedHorizontalOffset,
                    &zoomedVerticalOffset);
            }
        }
    }

    double anticipatedZoomedHorizontalOffset{ DoubleUtil::NaN };
    double anticipatedZoomedVerticalOffset{ DoubleUtil::NaN };

    switch (offsetsChange->ViewKind())
    {
#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
    case ScrollPresenterViewKind::RelativeToEndOfInertiaView:
    {
        const winrt::float2 endOfInertiaPosition = ComputeEndOfInertiaPosition();
        zoomedHorizontalOffset += endOfInertiaPosition.x;
        zoomedVerticalOffset += endOfInertiaPosition.y;
        break;
    }
#endif
    case ScrollPresenterViewKind::RelativeToCurrentView:
    {
        anticipatedZoomedHorizontalOffset = AnticipatedZoomedHorizontalOffset();
        anticipatedZoomedVerticalOffset = AnticipatedZoomedVerticalOffset();

        if (snapPointsMode == winrt::ScrollingSnapPointsMode::Default || animationMode == winrt::ScrollingAnimationMode::Enabled)
        {
            // The new requested deltas are added to the prior deltas that have not been processed yet.
            zoomedHorizontalOffset += anticipatedZoomedHorizontalOffset;
            zoomedVerticalOffset += anticipatedZoomedVerticalOffset;
        }
        break;
    }
    }

    if (snapPointsMode == winrt::ScrollingSnapPointsMode::Default)
    {
        zoomedHorizontalOffset = ComputeValueAfterSnapPoints<winrt::ScrollSnapPointBase>(zoomedHorizontalOffset, m_sortedConsolidatedHorizontalSnapPoints);
        zoomedVerticalOffset = ComputeValueAfterSnapPoints<winrt::ScrollSnapPointBase>(zoomedVerticalOffset, m_sortedConsolidatedVerticalSnapPoints);
    }

#ifdef DBG
    if (m_contentLayoutOffsetX != 0.0)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"contentLayoutOffsetX", m_contentLayoutOffsetX);
    }

    if (m_contentLayoutOffsetY != 0.0)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"contentLayoutOffsetY", m_contentLayoutOffsetY);
    }
#endif

    switch (animationMode)
    {
    case winrt::ScrollingAnimationMode::Disabled:
    {
        if (offsetsChange->ViewKind() == ScrollPresenterViewKind::RelativeToCurrentView && snapPointsMode == winrt::ScrollingSnapPointsMode::Ignore)
        {
            SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_METH_STR, METH_NAME,
                this,
                L"TryUpdatePositionBy",
                TypeLogging::Float2ToString(winrt::float2(static_cast<float>(zoomedHorizontalOffset), static_cast<float>(zoomedVerticalOffset))).c_str());

            TraceLoggingProviderWrite(
                XamlTelemetryLogging, "ScrollPresenter_TryUpdatePositionBy",
                TraceLoggingFloat64(zoomedHorizontalOffset, "HorizontalOffset"),
                TraceLoggingFloat64(zoomedVerticalOffset, "VerticalOffset"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdatePositionBy(
                winrt::float3(static_cast<float>(zoomedHorizontalOffset), static_cast<float>(zoomedVerticalOffset), 0.0f));
            m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdatePositionBy;

            if (zoomedHorizontalOffset != 0.0)
            {
                double newAnticipatedZoomedHorizontalOffset = zoomedHorizontalOffset + anticipatedZoomedHorizontalOffset;
                newAnticipatedZoomedHorizontalOffset = std::max(0.0, newAnticipatedZoomedHorizontalOffset);
                newAnticipatedZoomedHorizontalOffset = std::min(AnticipatedScrollableWidth(), newAnticipatedZoomedHorizontalOffset);
                UpdateAnticipatedOffset(ScrollPresenterDimension::HorizontalScroll, newAnticipatedZoomedHorizontalOffset);
            }

            if (zoomedVerticalOffset != 0.0)
            {
                double newAnticipatedZoomedVerticalOffset = zoomedVerticalOffset + anticipatedZoomedVerticalOffset;
                newAnticipatedZoomedVerticalOffset = std::max(0.0, newAnticipatedZoomedVerticalOffset);
                newAnticipatedZoomedVerticalOffset = std::min(AnticipatedScrollableHeight(), newAnticipatedZoomedVerticalOffset);
                UpdateAnticipatedOffset(ScrollPresenterDimension::VerticalScroll, newAnticipatedZoomedVerticalOffset);
            }
        }
        else
        {
            const winrt::float2 targetPosition = ComputePositionFromOffsets(zoomedHorizontalOffset, zoomedVerticalOffset);

            SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_METH_STR, METH_NAME, this,
                L"TryUpdatePosition", TypeLogging::Float2ToString(targetPosition).c_str());

            TraceLoggingProviderWrite(
                XamlTelemetryLogging, "ScrollPresenter_TryUpdatePosition",
                TraceLoggingFloat64(zoomedHorizontalOffset, "HorizontalOffset"),
                TraceLoggingFloat64(zoomedVerticalOffset, "VerticalOffset"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdatePosition(
                winrt::float3(targetPosition, 0.0f));
            m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdatePosition;

            double newAnticipatedZoomedHorizontalOffset = std::max(0.0, zoomedHorizontalOffset);
            newAnticipatedZoomedHorizontalOffset = std::min(AnticipatedScrollableWidth(), newAnticipatedZoomedHorizontalOffset);
            UpdateAnticipatedOffset(ScrollPresenterDimension::HorizontalScroll, newAnticipatedZoomedHorizontalOffset);

            double newAnticipatedZoomedVerticalOffset = std::max(0.0, zoomedVerticalOffset);
            newAnticipatedZoomedVerticalOffset = std::min(AnticipatedScrollableHeight(), newAnticipatedZoomedVerticalOffset);
            UpdateAnticipatedOffset(ScrollPresenterDimension::VerticalScroll, newAnticipatedZoomedVerticalOffset);
        }

        RaiseScrollStarting(
            offsetsChangeCorrelationId,
            AnticipatedZoomedHorizontalOffset(),
            AnticipatedZoomedVerticalOffset(),
            AnticipatedZoomFactor());

        if (isForAsyncOperation)
        {
            HookCompositionTargetRendering();
        }
        break;
    }
    case winrt::ScrollingAnimationMode::Enabled:
    {
        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"TryUpdatePositionWithAnimation");

        TraceLoggingProviderWrite(
            XamlTelemetryLogging, "ScrollPresenter_TryUpdatePositionWithAnimation",
            TraceLoggingFloat64(zoomedHorizontalOffset, "HorizontalOffset"),
            TraceLoggingFloat64(zoomedVerticalOffset, "VerticalOffset"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdatePositionWithAnimation(
            GetPositionAnimation(
                zoomedHorizontalOffset,
                zoomedVerticalOffset,
                operationTrigger,
                offsetsChangeCorrelationId));
        m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation;

        ResetAnticipatedView();
        break;
    }
    }
}

// Launches an InteractionTracker request to change the offsets with an additional velocity and optional scroll inertia decay rate.
void ScrollPresenter::ProcessOffsetsChange(
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    std::shared_ptr<OffsetsChangeWithAdditionalVelocity> offsetsChangeWithAdditionalVelocity)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(offsetsChangeWithAdditionalVelocity);

    winrt::float2 offsetsVelocity = offsetsChangeWithAdditionalVelocity->OffsetsVelocity();
    const winrt::IReference<winrt::float2> inertiaDecayRate = offsetsChangeWithAdditionalVelocity->InertiaDecayRate();

    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());

    if (operationTrigger == InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest ||
        operationTrigger == InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest)
    {
        // Requests coming from an IScrollController implementation do not include the 'minimum inertia velocity' value of 30.0f, because that
        // concept is InteractionTracker-specific (the IScrollController interface is meant to be InteractionTracker-agnostic).
        if (m_state != winrt::ScrollingInteractionState::Inertia)
        {
            // When there is no current inertia, include that minimum velocity automatically. So the IScrollController-provided velocity is always
            // proportional to the resulting offset change.
            static constexpr float s_minimumVelocity{ 30.0f };

            if (offsetsVelocity.x < 0.0f)
            {
                offsetsVelocity = { offsetsVelocity.x - s_minimumVelocity, offsetsVelocity.y };
            }
            else if (offsetsVelocity.x > 0.0f)
            {
                offsetsVelocity = { offsetsVelocity.x + s_minimumVelocity, offsetsVelocity.y };
            }

            if (offsetsVelocity.y < 0.0f)
            {
                offsetsVelocity = { offsetsVelocity.x, offsetsVelocity.y - s_minimumVelocity };
            }
            else if (offsetsVelocity.y > 0.0f)
            {
                offsetsVelocity = { offsetsVelocity.x, offsetsVelocity.y + s_minimumVelocity };
            }
        }
    }

    if (inertiaDecayRate)
    {
        const float horizontalInertiaDecayRate = std::clamp(inertiaDecayRate.Value().x, 0.0f, 1.0f);
        const float verticalInertiaDecayRate = std::clamp(inertiaDecayRate.Value().y, 0.0f, 1.0f);

        m_interactionTracker.PositionInertiaDecayRate(
            winrt::float3(horizontalInertiaDecayRate, verticalInertiaDecayRate, 0.0f));
    }
    else
    {
        // Restore the default 0.95 position inertia decay rate since it may have been overridden by a prior offset change with additional velocity.
        ResetOffsetsInertiaDecayRate();
    }

    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_METH_STR, METH_NAME, this,
        L"TryUpdatePositionWithAdditionalVelocity", TypeLogging::Float2ToString(winrt::float2(offsetsVelocity)).c_str());

    TraceLoggingProviderWrite(
        XamlTelemetryLogging, "ScrollPresenter_TryUpdatePositionWithAdditionalVelocity",
        TraceLoggingFloat32(offsetsVelocity.x, "Velocity.X"),
        TraceLoggingFloat32(offsetsVelocity.y, "Velocity.Y"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdatePositionWithAdditionalVelocity(
        winrt::float3(offsetsVelocity, 0.0f));
    m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity;

    ResetAnticipatedView();
}

// Restores the default scroll inertia decay rate if no offset change with additional velocity operation is in progress.
void ScrollPresenter::PostProcessOffsetsChange(
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, interactionTrackerAsyncOperation);

    MUX_ASSERT(m_interactionTracker);

    if (interactionTrackerAsyncOperation->GetRequestId() != m_latestInteractionTrackerRequest)
    {
        std::shared_ptr<InteractionTrackerAsyncOperation> latestInteractionTrackerAsyncOperation = GetInteractionTrackerOperationFromRequestId(
            m_latestInteractionTrackerRequest);
        if (latestInteractionTrackerAsyncOperation &&
            latestInteractionTrackerAsyncOperation->GetOperationType() == InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity)
        {
            // Do not reset the scroll inertia decay rate when there is a new ongoing offset change with additional velocity
            return;
        }
    }

    ResetOffsetsInertiaDecayRate();
}

// Launches an InteractionTracker request to change the zoomFactor.
void ScrollPresenter::ProcessZoomFactorChange(
    std::shared_ptr<ZoomFactorChange> zoomFactorChange,
    int32_t zoomFactorChangeCorrelationId)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(zoomFactorChange);

    float zoomFactor = zoomFactorChange->ZoomFactor();
    const winrt::IReference<winrt::float2> nullableCenterPoint = zoomFactorChange->CenterPoint();
    const ScrollPresenterViewKind viewKind = zoomFactorChange->ViewKind();
    const winrt::ScrollingZoomOptions options = zoomFactorChange->Options().try_as<winrt::ScrollingZoomOptions>();

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        TypeLogging::ScrollPresenterViewKindToString(viewKind).c_str(),
        zoomFactorChangeCorrelationId);
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(nullableCenterPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactor);

    const winrt::float2 centerPoint2D = nullableCenterPoint == nullptr ?
        winrt::float2(static_cast<float>(m_viewportWidth / 2.0), static_cast<float>(m_viewportHeight / 2.0)) : nullableCenterPoint.Value();
    const winrt::float3 centerPoint(centerPoint2D.x - m_contentLayoutOffsetX, centerPoint2D.y - m_contentLayoutOffsetY, 0.0f);

    switch (viewKind)
    {
#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
    case ScrollPresenterViewKind::RelativeToEndOfInertiaView:
    {
        zoomFactor += ComputeEndOfInertiaZoomFactor();
        break;
    }
#endif
    case ScrollPresenterViewKind::RelativeToCurrentView:
    {
        // The new requested delta is added to the prior deltas that have not been processed yet.
        zoomFactor += AnticipatedZoomFactor();
        break;
    }
    }

    winrt::ScrollingAnimationMode animationMode = options ? options.AnimationMode() : ScrollingScrollOptions::s_defaultAnimationMode;
    const winrt::ScrollingSnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollingScrollOptions::s_defaultSnapPointsMode;

    animationMode = GetComputedAnimationMode(animationMode);

    if (snapPointsMode == winrt::ScrollingSnapPointsMode::Default)
    {
        zoomFactor = static_cast<float>(ComputeValueAfterSnapPoints<winrt::ZoomSnapPointBase>(zoomFactor, m_sortedConsolidatedZoomSnapPoints));
    }

    switch (animationMode)
    {
    case winrt::ScrollingAnimationMode::Disabled:
    {
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_METH_FLT_STR, METH_NAME, this,
            L"TryUpdateScale", zoomFactor, TypeLogging::Float2ToString(winrt::float2(centerPoint.x, centerPoint.y)).c_str());

        TraceLoggingProviderWrite(
            XamlTelemetryLogging, "TryUpdateScale",
            TraceLoggingFloat32(zoomFactor, "ZoomFactor"),
            TraceLoggingFloat32(centerPoint.x, "Center.X"),
            TraceLoggingFloat32(centerPoint.y, "Center.Y"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdateScale(zoomFactor, centerPoint);
        m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdateScale;

        double newAnticipatedZoomedHorizontalOffset = zoomFactor / AnticipatedZoomFactor() * (AnticipatedZoomedHorizontalOffset() + centerPoint.x) - centerPoint.x;
        double newAnticipatedZoomedVerticalOffset = zoomFactor / AnticipatedZoomFactor() * (AnticipatedZoomedVerticalOffset() + centerPoint.y) - centerPoint.y;

        UpdateAnticipatedZoomFactor(zoomFactor);

        newAnticipatedZoomedHorizontalOffset = std::max(0.0, newAnticipatedZoomedHorizontalOffset);
        newAnticipatedZoomedHorizontalOffset = std::min(AnticipatedScrollableWidth(), newAnticipatedZoomedHorizontalOffset);
        UpdateAnticipatedOffset(ScrollPresenterDimension::HorizontalScroll, newAnticipatedZoomedHorizontalOffset);

        newAnticipatedZoomedVerticalOffset = std::max(0.0, newAnticipatedZoomedVerticalOffset);
        newAnticipatedZoomedVerticalOffset = std::min(AnticipatedScrollableHeight(), newAnticipatedZoomedVerticalOffset);
        UpdateAnticipatedOffset(ScrollPresenterDimension::VerticalScroll, newAnticipatedZoomedVerticalOffset);

        RaiseZoomStarting(
            zoomFactorChangeCorrelationId,
            newAnticipatedZoomedHorizontalOffset,
            newAnticipatedZoomedVerticalOffset,
            zoomFactor);

        HookCompositionTargetRendering();
        break;
    }
    case winrt::ScrollingAnimationMode::Enabled:
    {
        SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"TryUpdateScaleWithAnimation");

        TraceLoggingProviderWrite(
            XamlTelemetryLogging, "TryUpdateScaleWithAnimation",
            TraceLoggingFloat32(zoomFactor, "ZoomFactor"),
            TraceLoggingFloat32(centerPoint.x, "Center.X"),
            TraceLoggingFloat32(centerPoint.y, "Center.Y"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdateScaleWithAnimation(
            GetZoomFactorAnimation(zoomFactor, centerPoint2D, zoomFactorChangeCorrelationId),
            centerPoint);
        m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation;

        ResetAnticipatedView();
        break;
    }
    }
}

// Launches an InteractionTracker request to change the zoomFactor with an additional velocity and an optional zoomFactor inertia decay rate.
void ScrollPresenter::ProcessZoomFactorChange(
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    std::shared_ptr<ZoomFactorChangeWithAdditionalVelocity> zoomFactorChangeWithAdditionalVelocity)
{
    MUX_ASSERT(m_interactionTracker);
    MUX_ASSERT(zoomFactorChangeWithAdditionalVelocity);

    float zoomFactorVelocity = zoomFactorChangeWithAdditionalVelocity->ZoomFactorVelocity();
    winrt::IReference<float> inertiaDecayRate = zoomFactorChangeWithAdditionalVelocity->InertiaDecayRate();
    winrt::IReference<winrt::float2> nullableCenterPoint = zoomFactorChangeWithAdditionalVelocity->CenterPoint();

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::InteractionTrackerAsyncOperationTriggerToString(operationTrigger).c_str());
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(nullableCenterPoint).c_str(),
        TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(),
        zoomFactorVelocity);

    if (inertiaDecayRate)
    {
        const float scaleInertiaDecayRate = std::clamp(inertiaDecayRate.Value(), 0.0f, 1.0f);

        m_interactionTracker.ScaleInertiaDecayRate(scaleInertiaDecayRate);
    }
    else
    {
        // Restore the default 0.985 zoomFactor inertia decay rate since it may have been overridden by a prior zoomFactor change with additional velocity.
        ResetZoomFactorInertiaDecayRate();
    }

    const winrt::float2 centerPoint2D = nullableCenterPoint == nullptr ?
        winrt::float2(static_cast<float>(m_viewportWidth / 2.0), static_cast<float>(m_viewportHeight / 2.0)) : nullableCenterPoint.Value();
    const winrt::float3 centerPoint(centerPoint2D.x - m_contentLayoutOffsetX, centerPoint2D.y - m_contentLayoutOffsetY, 0.0f);

    SCROLLPRESENTER_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_METH_FLT_STR, METH_NAME, this,
        L"TryUpdateScaleWithAdditionalVelocity", zoomFactorVelocity, TypeLogging::Float2ToString(winrt::float2(centerPoint.x, centerPoint.y)).c_str());

    TraceLoggingProviderWrite(
        XamlTelemetryLogging, "TryUpdateScaleWithAdditionalVelocity",
        TraceLoggingFloat32(zoomFactorVelocity, "Velocity"),
        TraceLoggingFloat32(centerPoint.x, "Center.X"),
        TraceLoggingFloat32(centerPoint.y, "Center.Y"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    m_latestInteractionTrackerRequest = m_interactionTracker.TryUpdateScaleWithAdditionalVelocity(
        zoomFactorVelocity,
        centerPoint);
    m_lastInteractionTrackerAsyncOperationType = InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity;

    ResetAnticipatedView();
}

// Restores the default zoomFactor inertia decay rate if no zoomFactor change with additional velocity operation is in progress.
void ScrollPresenter::PostProcessZoomFactorChange(
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, interactionTrackerAsyncOperation);

    MUX_ASSERT(m_interactionTracker);

    if (interactionTrackerAsyncOperation->GetRequestId() != m_latestInteractionTrackerRequest)
    {
        std::shared_ptr<InteractionTrackerAsyncOperation> latestInteractionTrackerAsyncOperation = GetInteractionTrackerOperationFromRequestId(
            m_latestInteractionTrackerRequest);
        if (latestInteractionTrackerAsyncOperation &&
            latestInteractionTrackerAsyncOperation->GetOperationType() == InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity)
        {
            // Do not reset the zoomFactor inertia decay rate when there is a new ongoing zoomFactor change with additional velocity
            return;
        }
    }

    ResetZoomFactorInertiaDecayRate();
}

// Clears the last recorded anticipated view for the ScrollStarting/ZoomStarting events.
// Called in two classes of circumstances:
// - all queued view change requested were completed,
// - an animated view change request is handed off to the InteractionTracker.
void ScrollPresenter::ResetAnticipatedView()
{
    UpdateAnticipatedOffset(ScrollPresenterDimension::HorizontalScroll, DoubleUtil::NaN);
    UpdateAnticipatedOffset(ScrollPresenterDimension::VerticalScroll, DoubleUtil::NaN);
    UpdateAnticipatedZoomFactor(FloatUtil::NaN);
}

// Restores the default scroll offset inertia decay rate.
void ScrollPresenter::ResetOffsetsInertiaDecayRate()
{
    MUX_ASSERT(m_interactionTracker);

#ifdef DBG
    const winrt::IReference<winrt::float3> inertiaDecayRateDbg = m_interactionTracker.PositionInertiaDecayRate();

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        L"PositionInertiaDecayRate",
        inertiaDecayRateDbg ? TypeLogging::Float2ToString(winrt::float2{ inertiaDecayRateDbg.Value().x, inertiaDecayRateDbg.Value().y }).c_str() : L"null");
#endif

    m_interactionTracker.PositionInertiaDecayRate(nullptr);
}

// Restores the default zoomFactor inertia decay rate.
void ScrollPresenter::ResetZoomFactorInertiaDecayRate()
{
    MUX_ASSERT(m_interactionTracker);

#ifdef DBG
    const winrt::IReference<float> inertiaDecayRateDbg = m_interactionTracker.ScaleInertiaDecayRate();

    if (inertiaDecayRateDbg)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"ScaleInertiaDecayRate", inertiaDecayRateDbg.Value());
    }
    else
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"ScaleInertiaDecayRate", L"null");
    }
#endif

    m_interactionTracker.ScaleInertiaDecayRate(nullptr);
}

void ScrollPresenter::CompleteViewChange(
    std::shared_ptr<InteractionTrackerAsyncOperation> interactionTrackerAsyncOperation,
    ScrollPresenterViewChangeResult result)
{
    const int32_t viewChangeCorrelationId = interactionTrackerAsyncOperation->GetViewChangeCorrelationId();

    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this,
        interactionTrackerAsyncOperation.get(), TypeLogging::ScrollPresenterViewChangeResultToString(result).c_str());
    SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        L"viewChangeCorrelationId", viewChangeCorrelationId);

    interactionTrackerAsyncOperation->SetIsCompleted(true);

    bool onHorizontalOffsetChangeCompleted = false;
    bool onVerticalOffsetChangeCompleted = false;

    switch (static_cast<int>(interactionTrackerAsyncOperation->GetOperationTrigger()))
    {
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::DirectViewChange) :
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::BringIntoViewRequest):
            switch (interactionTrackerAsyncOperation->GetOperationType())
            {
            case InteractionTrackerAsyncOperationType::TryUpdatePosition:
            case InteractionTrackerAsyncOperationType::TryUpdatePositionBy:
            case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation:
            case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
                RaiseViewChangeCompleted(true /*isForScroll*/, result, viewChangeCorrelationId);
                break;
            default:
                // Stop Translation and Scale animations if needed, to trigger rasterization of Content & avoid fuzzy text rendering for instance.
                StopTranslationAndZoomFactorExpressionAnimations();

                RaiseViewChangeCompleted(false /*isForScroll*/, result, viewChangeCorrelationId);
                break;
            }
        break;
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) :
            onHorizontalOffsetChangeCompleted = true;
            RaiseViewChangeCompleted(true /*isForScroll*/, result, viewChangeCorrelationId);
            break;
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest) :
            onVerticalOffsetChangeCompleted = true;
            RaiseViewChangeCompleted(true /*isForScroll*/, result, viewChangeCorrelationId);
            break;
        case static_cast<int>(InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest) |
             static_cast<int>(InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest) :
            onHorizontalOffsetChangeCompleted = true;
            onVerticalOffsetChangeCompleted = true;
            RaiseViewChangeCompleted(true /*isForScroll*/, result, viewChangeCorrelationId);
            break;
    }

    if (onHorizontalOffsetChangeCompleted && m_horizontalScrollController)
    {
        m_horizontalScrollController.get().NotifyRequestedScrollCompleted(viewChangeCorrelationId);
    }

    if (onVerticalOffsetChangeCompleted && m_verticalScrollController)
    {
        m_verticalScrollController.get().NotifyRequestedScrollCompleted(viewChangeCorrelationId);
    }
}

void ScrollPresenter::CompleteInteractionTrackerOperations(
    int requestId,
    ScrollPresenterViewChangeResult operationResult,
    ScrollPresenterViewChangeResult priorNonAnimatedOperationsResult,
    ScrollPresenterViewChangeResult priorAnimatedOperationsResult,
    bool completeNonAnimatedOperation,
    bool completeAnimatedOperation,
    bool completePriorNonAnimatedOperations,
    bool completePriorAnimatedOperations)
{
    MUX_ASSERT(requestId != 0);
    MUX_ASSERT(completeNonAnimatedOperation || completeAnimatedOperation || completePriorNonAnimatedOperations || completePriorAnimatedOperations);

    if (m_interactionTrackerAsyncOperations.empty())
    {
        return;
    }

    for (auto operationsIter = m_interactionTrackerAsyncOperations.begin(); operationsIter != m_interactionTrackerAsyncOperations.end();)
    {
        const auto interactionTrackerAsyncOperation = *operationsIter;

        operationsIter++;

        const bool isMatch = requestId == -1 || requestId == interactionTrackerAsyncOperation->GetRequestId();
        const bool isPriorMatch = requestId > interactionTrackerAsyncOperation->GetRequestId() && -1 != interactionTrackerAsyncOperation->GetRequestId();

        if ((isPriorMatch && (completePriorNonAnimatedOperations || completePriorAnimatedOperations)) ||
            (isMatch && (completeNonAnimatedOperation || completeAnimatedOperation)))
        {
            const bool isOperationAnimated = interactionTrackerAsyncOperation->IsAnimated();
            const bool complete =
                (isMatch && completeNonAnimatedOperation && !isOperationAnimated) ||
                (isMatch && completeAnimatedOperation && isOperationAnimated) ||
                (isPriorMatch && completePriorNonAnimatedOperations && !isOperationAnimated) ||
                (isPriorMatch && completePriorAnimatedOperations && isOperationAnimated);

            if (complete)
            {
                CompleteViewChange(
                    interactionTrackerAsyncOperation,
                    isMatch ? operationResult : (isOperationAnimated ? priorAnimatedOperationsResult : priorNonAnimatedOperationsResult));

                auto interactionTrackerAsyncOperationRemoved = interactionTrackerAsyncOperation;

                m_interactionTrackerAsyncOperations.remove(interactionTrackerAsyncOperationRemoved);

                if (m_interactionTrackerAsyncOperations.empty())
                {
                    ResetAnticipatedView();
                }

                switch (interactionTrackerAsyncOperationRemoved->GetOperationType())
                {
                case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
                    PostProcessOffsetsChange(interactionTrackerAsyncOperationRemoved);
                    break;
                case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity:
                    PostProcessZoomFactorChange(interactionTrackerAsyncOperationRemoved);
                    break;
                }
            }
        }
    }
}

void ScrollPresenter::CompleteDelayedOperations()
{
    if (m_interactionTrackerAsyncOperations.empty())
    {
        return;
    }

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    for (auto operationsIter = m_interactionTrackerAsyncOperations.begin(); operationsIter != m_interactionTrackerAsyncOperations.end();)
    {
        auto& interactionTrackerAsyncOperation = *operationsIter;

        operationsIter++;

        if (interactionTrackerAsyncOperation->IsDelayed())
        {
            CompleteViewChange(interactionTrackerAsyncOperation, ScrollPresenterViewChangeResult::Interrupted);
            m_interactionTrackerAsyncOperations.remove(interactionTrackerAsyncOperation);

            if (m_interactionTrackerAsyncOperations.empty())
            {
                ResetAnticipatedView();
            }
        }
    }
}

// Sets the ticks countdown of the queued operations to the max value of InteractionTrackerAsyncOperation::c_queuedOperationTicks == 3.
// Invoked when the extent or viewport size changed in order to let it propagate to the Composition thread and thus let the
// InteractionTracker operate on the latest sizes.
void ScrollPresenter::MaximizeInteractionTrackerOperationsTicksCountdown()
{
    if (m_interactionTrackerAsyncOperations.empty())
    {
        return;
    }

    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    for (auto operationsIter = m_interactionTrackerAsyncOperations.begin(); operationsIter != m_interactionTrackerAsyncOperations.end();)
    {
        auto& interactionTrackerAsyncOperation = *operationsIter;

        operationsIter++;

        if (!interactionTrackerAsyncOperation->IsDelayed() &&
            !interactionTrackerAsyncOperation->IsCanceled() &&
            !interactionTrackerAsyncOperation->IsCompleted() &&
            interactionTrackerAsyncOperation->IsQueued())
        {
            interactionTrackerAsyncOperation->SetMaxTicksCountdown();
        }
    }
}

// Returns the maximum remaining ticks countdown of all queued operations.
// Used by ScrollPresenter::ChangeOffsetsPrivate, ScrollPresenter::ChangeOffsetsWithAdditionalVelocityPrivate and ScrollPresenter::ChangeZoomFactorWithAdditionalVelocityPrivate
// to make sure newly queued operations do not get processed before existing ones.
int ScrollPresenter::GetInteractionTrackerOperationsTicksCountdown() const
{
    int ticksCountdown = 0;

    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        if (!interactionTrackerAsyncOperation->IsCompleted() &&
            !interactionTrackerAsyncOperation->IsCanceled())
        {
            ticksCountdown = std::max(ticksCountdown, interactionTrackerAsyncOperation->GetTicksCountdown());
        }
    }

    return ticksCountdown;
}

int ScrollPresenter::GetInteractionTrackerOperationsCount(bool includeAnimatedOperations, bool includeNonAnimatedOperations) const
{
    MUX_ASSERT(includeAnimatedOperations || includeNonAnimatedOperations);

    int operationsCount = 0;

    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        const bool isOperationAnimated = interactionTrackerAsyncOperation->IsAnimated();

        if ((isOperationAnimated && includeAnimatedOperations) || (!isOperationAnimated && includeNonAnimatedOperations))
        {
            operationsCount++;
        }
    }

    return operationsCount;
}

std::shared_ptr<InteractionTrackerAsyncOperation> ScrollPresenter::GetLastNonAnimatedInteractionTrackerOperation(
    std::shared_ptr<InteractionTrackerAsyncOperation> priorToInteractionTrackerOperation) const
{
    bool priorInteractionTrackerOperationSeen = false;

    for (auto operationsIter = m_interactionTrackerAsyncOperations.end(); operationsIter != m_interactionTrackerAsyncOperations.begin();)
    {
        operationsIter--;

        auto& interactionTrackerAsyncOperation = *operationsIter;

        if (!priorInteractionTrackerOperationSeen && priorToInteractionTrackerOperation == interactionTrackerAsyncOperation)
        {
            priorInteractionTrackerOperationSeen = true;
        }
        else if (priorInteractionTrackerOperationSeen &&
            !interactionTrackerAsyncOperation->IsAnimated() &&
            !interactionTrackerAsyncOperation->IsCompleted() &&
            !interactionTrackerAsyncOperation->IsCanceled())
        {
            MUX_ASSERT(interactionTrackerAsyncOperation->IsDelayed() || interactionTrackerAsyncOperation->IsQueued());
            return interactionTrackerAsyncOperation;
        }
    }

    return nullptr;
}

std::shared_ptr<InteractionTrackerAsyncOperation> ScrollPresenter::GetInteractionTrackerOperationFromRequestId(int requestId) const
{
    MUX_ASSERT(requestId >= 0);

    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        if (interactionTrackerAsyncOperation->GetRequestId() == requestId)
        {
            return interactionTrackerAsyncOperation;
        }
    }

    return nullptr;
}

std::shared_ptr<InteractionTrackerAsyncOperation> ScrollPresenter::GetInteractionTrackerOperationFromKinds(
    bool isOperationTypeForOffsetsChange,
    InteractionTrackerAsyncOperationTrigger operationTrigger,
    ScrollPresenterViewKind const& viewKind,
    winrt::ScrollingScrollOptions const& options) const
{
    // Going through the existing operations from most recent to oldest, trying to find a match for the trigger, kind and options.
    for (auto operationsIter = m_interactionTrackerAsyncOperations.end(); operationsIter != m_interactionTrackerAsyncOperations.begin();)
    {
        operationsIter--;

        auto& interactionTrackerAsyncOperation = *operationsIter;

        if ((static_cast<int>(interactionTrackerAsyncOperation->GetOperationTrigger()) & static_cast<int>(operationTrigger)) == 0x00 &&
            !interactionTrackerAsyncOperation->IsCanceled())
        {
            // When a non-canceled operation with a different trigger is encountered, we bail out right away.
            return nullptr;
        }

        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();

        if ((static_cast<int>(interactionTrackerAsyncOperation->GetOperationTrigger()) & static_cast<int>(operationTrigger)) == 0x00 ||
            !interactionTrackerAsyncOperation->IsQueued() ||
            interactionTrackerAsyncOperation->IsUnqueueing() ||
            interactionTrackerAsyncOperation->IsCanceled() ||
            !viewChangeBase)
        {
            continue;
        }

        switch (interactionTrackerAsyncOperation->GetOperationType())
        {
        case InteractionTrackerAsyncOperationType::TryUpdatePosition:
        case InteractionTrackerAsyncOperationType::TryUpdatePositionBy:
        case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation:
        {
            if (!isOperationTypeForOffsetsChange)
            {
                continue;
            }

            std::shared_ptr<ViewChange> viewChange = std::reinterpret_pointer_cast<ViewChange>(viewChangeBase);

            if (viewChange->ViewKind() != viewKind)
            {
                continue;
            }

            const winrt::ScrollingScrollOptions optionsClone = viewChange->Options().try_as<winrt::ScrollingScrollOptions>();
            const winrt::ScrollingAnimationMode animationMode = options ? options.AnimationMode() : ScrollingScrollOptions::s_defaultAnimationMode;
            const winrt::ScrollingAnimationMode animationModeClone = optionsClone ? optionsClone.AnimationMode() : ScrollingScrollOptions::s_defaultAnimationMode;

            if (animationModeClone != animationMode)
            {
                continue;
            }

            const winrt::ScrollingSnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollingScrollOptions::s_defaultSnapPointsMode;
            const winrt::ScrollingSnapPointsMode snapPointsModeClone = optionsClone ? optionsClone.SnapPointsMode() : ScrollingScrollOptions::s_defaultSnapPointsMode;

            if (snapPointsModeClone != snapPointsMode)
            {
                continue;
            }
            break;
        }
        case InteractionTrackerAsyncOperationType::TryUpdateScale:
        case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation:
        {
            if (isOperationTypeForOffsetsChange)
            {
                continue;
            }

            const std::shared_ptr<ViewChange> viewChange = std::reinterpret_pointer_cast<ViewChange>(viewChangeBase);

            if (viewChange->ViewKind() != viewKind)
            {
                continue;
            }

            const winrt::ScrollingZoomOptions optionsClone = viewChange->Options().try_as<winrt::ScrollingZoomOptions>();
            const winrt::ScrollingAnimationMode animationMode = options ? options.AnimationMode() : ScrollingScrollOptions::s_defaultAnimationMode;
            const winrt::ScrollingAnimationMode animationModeClone = optionsClone ? optionsClone.AnimationMode() : ScrollingScrollOptions::s_defaultAnimationMode;

            if (animationModeClone != animationMode)
            {
                continue;
            }

            const winrt::ScrollingSnapPointsMode snapPointsMode = options ? options.SnapPointsMode() : ScrollingScrollOptions::s_defaultSnapPointsMode;
            const winrt::ScrollingSnapPointsMode snapPointsModeClone = optionsClone ? optionsClone.SnapPointsMode() : ScrollingScrollOptions::s_defaultSnapPointsMode;

            if (snapPointsModeClone != snapPointsMode)
            {
                continue;
            }
            break;
        }
        }

        return interactionTrackerAsyncOperation;
    }

    return nullptr;
}

std::shared_ptr<InteractionTrackerAsyncOperation> ScrollPresenter::GetInteractionTrackerOperationWithAdditionalVelocity(
    bool isOperationTypeForOffsetsChange,
    InteractionTrackerAsyncOperationTrigger operationTrigger) const
{
    for (auto& interactionTrackerAsyncOperation : m_interactionTrackerAsyncOperations)
    {
        std::shared_ptr<ViewChangeBase> viewChangeBase = interactionTrackerAsyncOperation->GetViewChangeBase();

        if ((static_cast<int>(interactionTrackerAsyncOperation->GetOperationTrigger()) & static_cast<int>(operationTrigger)) == 0x00 ||
            !interactionTrackerAsyncOperation->IsQueued() ||
            interactionTrackerAsyncOperation->IsUnqueueing() ||
            interactionTrackerAsyncOperation->IsCanceled() ||
            !viewChangeBase)
        {
            continue;
        }

        switch (interactionTrackerAsyncOperation->GetOperationType())
        {
        case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
        {
            if (!isOperationTypeForOffsetsChange)
            {
                continue;
            }
            return interactionTrackerAsyncOperation;
        }
        case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity:
        {
            if (isOperationTypeForOffsetsChange)
            {
                continue;
            }
            return interactionTrackerAsyncOperation;
        }
        }
    }

    return nullptr;
}

template <typename T>
winrt::InteractionTrackerInertiaRestingValue ScrollPresenter::GetInertiaRestingValue(
    std::shared_ptr<SnapPointWrapper<T>> snapPointWrapper,
    winrt::Compositor const& compositor,
    winrt::hstring const& target,
    winrt::hstring const& scale) const
{
    const bool isInertiaFromImpulse = IsInertiaFromImpulse();
    const winrt::InteractionTrackerInertiaRestingValue modifier = winrt::InteractionTrackerInertiaRestingValue::Create(compositor);
    const winrt::ExpressionAnimation conditionExpressionAnimation = snapPointWrapper->CreateConditionalExpression(m_interactionTracker, target, scale, isInertiaFromImpulse);
    const winrt::ExpressionAnimation restingPointExpressionAnimation = snapPointWrapper->CreateRestingPointExpression(m_interactionTracker, target, scale, isInertiaFromImpulse);

    modifier.Condition(conditionExpressionAnimation);
    modifier.RestingValue(restingPointExpressionAnimation);

    return modifier;
}

// Relies on InteractionTracker.IsInertiaFromImpulse starting with RS5,
// returns the replacement field m_isInertiaFromImpulse otherwise.
bool ScrollPresenter::IsInertiaFromImpulse() const
{
    if (winrt::IInteractionTracker4 interactionTracker4 = m_interactionTracker)
    {
        return interactionTracker4.IsInertiaFromImpulse();
    }
    else
    {
        return m_isInertiaFromImpulse;
    }
}

bool ScrollPresenter::IsLoadedAndSetUp() const
{
    return IsLoaded() && m_interactionTracker;
}

bool ScrollPresenter::IsInputKindIgnored(winrt::ScrollingInputKinds const& inputKind)
{
    return (IgnoredInputKinds() & inputKind) == inputKind;
}

void ScrollPresenter::HookCompositionTargetRendering()
{
    if (!m_renderingRevoker)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

        winrt::Microsoft::UI::Xaml::Media::CompositionTarget compositionTarget{ nullptr };
        m_renderingRevoker = compositionTarget.Rendering(winrt::auto_revoke, { this, &ScrollPresenter::OnCompositionTargetRendering });
    }
}

void ScrollPresenter::UnhookCompositionTargetRendering()
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_renderingRevoker.revoke();
}

void ScrollPresenter::HookScrollPresenterEvents()
{
    if (!m_flowDirectionChangedRevoker)
    {
        m_flowDirectionChangedRevoker = RegisterPropertyChanged(*this, winrt::FrameworkElement::FlowDirectionProperty(), { this, &ScrollPresenter::OnFlowDirectionChanged });
    }

    if (!m_loadedRevoker)
    {
        m_loadedRevoker = Loaded(winrt::auto_revoke, { this, &ScrollPresenter::OnLoaded });
    }

    if (!m_unloadedRevoker)
    {
        m_unloadedRevoker = Unloaded(winrt::auto_revoke, { this, &ScrollPresenter::OnUnloaded });
    }

    if (!m_bringIntoViewRequestedRevoker)
    {
        m_bringIntoViewRequestedRevoker = BringIntoViewRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnBringIntoViewRequestedHandler });
    }

    if (!m_pointerPressedEventHandler)
    {
        m_pointerPressedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollPresenter::OnPointerPressed });
        MUX_ASSERT(m_pointerPressedEventHandler);
        AddHandler(winrt::UIElement::PointerPressedEvent(), m_pointerPressedEventHandler, true /*handledEventsToo*/);
    }
}

void ScrollPresenter::UnhookScrollPresenterEvents()
{
    m_flowDirectionChangedRevoker.revoke();
    m_loadedRevoker.revoke();
    m_unloadedRevoker.revoke();
    m_bringIntoViewRequestedRevoker.revoke();

    if (m_pointerPressedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_pointerPressedEventHandler);
        m_pointerPressedEventHandler = nullptr;
    }
}

void ScrollPresenter::HookContentPropertyChanged(
    const winrt::UIElement& content)
{
    if (content)
    {
        if (auto contentAsFE = content.try_as<winrt::FrameworkElement>())
        {
            if (!m_contentMinWidthChangedRevoker)
            {
                m_contentMinWidthChangedRevoker = RegisterPropertyChanged(contentAsFE, winrt::FrameworkElement::MinWidthProperty(), { this, &ScrollPresenter::OnContentPropertyChanged });
            }
            if (!m_contentWidthChangedRevoker)
            {
                m_contentWidthChangedRevoker = RegisterPropertyChanged(contentAsFE, winrt::FrameworkElement::WidthProperty(), { this, &ScrollPresenter::OnContentPropertyChanged });
            }
            if (!m_contentMaxWidthChangedRevoker)
            {
                m_contentMaxWidthChangedRevoker = RegisterPropertyChanged(contentAsFE, winrt::FrameworkElement::MaxWidthProperty(), { this, &ScrollPresenter::OnContentPropertyChanged });
            }
            if (!m_contentMinHeightChangedRevoker)
            {
                m_contentMinHeightChangedRevoker = RegisterPropertyChanged(contentAsFE, winrt::FrameworkElement::MinHeightProperty(), { this, &ScrollPresenter::OnContentPropertyChanged });
            }
            if (!m_contentHeightChangedRevoker)
            {
                m_contentHeightChangedRevoker = RegisterPropertyChanged(contentAsFE, winrt::FrameworkElement::HeightProperty(), { this, &ScrollPresenter::OnContentPropertyChanged });
            }
            if (!m_contentMaxHeightChangedRevoker)
            {
                m_contentMaxHeightChangedRevoker = RegisterPropertyChanged(contentAsFE, winrt::FrameworkElement::MaxHeightProperty(), { this, &ScrollPresenter::OnContentPropertyChanged });
            }
            if (!m_contentHorizontalAlignmentChangedRevoker)
            {
                m_contentHorizontalAlignmentChangedRevoker = RegisterPropertyChanged(contentAsFE, winrt::FrameworkElement::HorizontalAlignmentProperty(), { this, &ScrollPresenter::OnContentPropertyChanged });
            }
            if (!m_contentVerticalAlignmentChangedRevoker)
            {
                m_contentVerticalAlignmentChangedRevoker = RegisterPropertyChanged(contentAsFE, winrt::FrameworkElement::VerticalAlignmentProperty(), { this, &ScrollPresenter::OnContentPropertyChanged });
            }
        }
    }
}

void ScrollPresenter::UnhookContentPropertyChanged(
    const winrt::UIElement& content)
{
    if (content)
    {
        const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();

        if (contentAsFE)
        {
            m_contentMinWidthChangedRevoker.revoke();
            m_contentWidthChangedRevoker.revoke();
            m_contentMaxWidthChangedRevoker.revoke();
            m_contentMinHeightChangedRevoker.revoke();
            m_contentHeightChangedRevoker.revoke();
            m_contentMaxHeightChangedRevoker.revoke();
            m_contentHorizontalAlignmentChangedRevoker.revoke();
            m_contentVerticalAlignmentChangedRevoker.revoke();
        }
    }
}

void ScrollPresenter::HookHorizontalScrollControllerEvents(
    const winrt::IScrollController& horizontalScrollController)
{
    MUX_ASSERT(horizontalScrollController);

    if (!m_horizontalScrollControllerScrollToRequestedRevoker)
    {
        m_horizontalScrollControllerScrollToRequestedRevoker = horizontalScrollController.ScrollToRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerScrollToRequested });
    }

    if (!m_horizontalScrollControllerScrollByRequestedRevoker)
    {
        m_horizontalScrollControllerScrollByRequestedRevoker = horizontalScrollController.ScrollByRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerScrollByRequested });
    }

    if (!m_horizontalScrollControllerAddScrollVelocityRequestedRevoker)
    {
        m_horizontalScrollControllerAddScrollVelocityRequestedRevoker = horizontalScrollController.AddScrollVelocityRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerAddScrollVelocityRequested });
    }
}

void ScrollPresenter::HookHorizontalScrollControllerPanningInfoEvents(
    const winrt::IScrollControllerPanningInfo& horizontalScrollControllerPanningInfo,
    bool hasInteractionSource)
{
    MUX_ASSERT(horizontalScrollControllerPanningInfo);

    if (hasInteractionSource)
    {
        HookHorizontalScrollControllerInteractionSourceEvents(horizontalScrollControllerPanningInfo);
    }

    if (!m_horizontalScrollControllerPanningInfoChangedRevoker)
    {
        m_horizontalScrollControllerPanningInfoChangedRevoker = horizontalScrollControllerPanningInfo.Changed(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerPanningInfoChanged });
    }
}

void ScrollPresenter::HookHorizontalScrollControllerInteractionSourceEvents(
    const winrt::IScrollControllerPanningInfo& horizontalScrollControllerPanningInfo)
{
    MUX_ASSERT(horizontalScrollControllerPanningInfo);

    if (!m_horizontalScrollControllerPanningInfoPanRequestedRevoker)
    {
        m_horizontalScrollControllerPanningInfoPanRequestedRevoker = horizontalScrollControllerPanningInfo.PanRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerPanningInfoPanRequested });
    }
}

void ScrollPresenter::HookVerticalScrollControllerEvents(
    const winrt::IScrollController& verticalScrollController)
{
    MUX_ASSERT(verticalScrollController);

    if (!m_verticalScrollControllerScrollToRequestedRevoker)
    {
        m_verticalScrollControllerScrollToRequestedRevoker = verticalScrollController.ScrollToRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerScrollToRequested });
    }

    if (!m_verticalScrollControllerScrollByRequestedRevoker)
    {
        m_verticalScrollControllerScrollByRequestedRevoker = verticalScrollController.ScrollByRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerScrollByRequested });
    }

    if (!m_verticalScrollControllerAddScrollVelocityRequestedRevoker)
    {
        m_verticalScrollControllerAddScrollVelocityRequestedRevoker = verticalScrollController.AddScrollVelocityRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerAddScrollVelocityRequested });
    }
}

void ScrollPresenter::HookVerticalScrollControllerPanningInfoEvents(
    const winrt::IScrollControllerPanningInfo& verticalScrollControllerPanningInfo,
    bool hasInteractionSource)
{
    MUX_ASSERT(verticalScrollControllerPanningInfo);

    if (hasInteractionSource)
    {
        HookVerticalScrollControllerInteractionSourceEvents(verticalScrollControllerPanningInfo);
    }

    if (!m_verticalScrollControllerPanningInfoChangedRevoker)
    {
        m_verticalScrollControllerPanningInfoChangedRevoker = verticalScrollControllerPanningInfo.Changed(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerPanningInfoChanged });
    }
}

void ScrollPresenter::HookVerticalScrollControllerInteractionSourceEvents(
    const winrt::IScrollControllerPanningInfo& verticalScrollControllerPanningInfo)
{
    MUX_ASSERT(verticalScrollControllerPanningInfo);

    if (!m_verticalScrollControllerPanningInfoPanRequestedRevoker)
    {
        m_verticalScrollControllerPanningInfoPanRequestedRevoker = verticalScrollControllerPanningInfo.PanRequested(winrt::auto_revoke, { this, &ScrollPresenter::OnScrollControllerPanningInfoPanRequested });
    }
}

void ScrollPresenter::UnhookHorizontalScrollControllerEvents()
{
    m_horizontalScrollControllerScrollToRequestedRevoker.revoke();
    m_horizontalScrollControllerScrollByRequestedRevoker.revoke();
    m_horizontalScrollControllerAddScrollVelocityRequestedRevoker.revoke();
}

void ScrollPresenter::UnhookHorizontalScrollControllerPanningInfoEvents()
{
    m_horizontalScrollControllerPanningInfoChangedRevoker.revoke();
    m_horizontalScrollControllerPanningInfoPanRequestedRevoker.revoke();
}

void ScrollPresenter::UnhookVerticalScrollControllerEvents()
{
    m_verticalScrollControllerScrollToRequestedRevoker.revoke();
    m_verticalScrollControllerScrollByRequestedRevoker.revoke();
    m_verticalScrollControllerAddScrollVelocityRequestedRevoker.revoke();
}

void ScrollPresenter::UnhookVerticalScrollControllerPanningInfoEvents()
{
    m_verticalScrollControllerPanningInfoChangedRevoker.revoke();
    m_verticalScrollControllerPanningInfoPanRequestedRevoker.revoke();
}

void ScrollPresenter::RaiseInteractionSourcesChanged()
{
    com_ptr<ScrollPresenterTestHooks> globalTestHooks = ScrollPresenterTestHooks::GetGlobalTestHooks();

    if (globalTestHooks && globalTestHooks->AreInteractionSourcesNotificationsRaised())
    {
        globalTestHooks->NotifyInteractionSourcesChanged(*this, m_interactionTracker.InteractionSources());
    }
}

void ScrollPresenter::RaiseExpressionAnimationStatusChanged(
    bool isExpressionAnimationStarted,
    wstring_view const& propertyName)
{
    com_ptr<ScrollPresenterTestHooks> globalTestHooks = ScrollPresenterTestHooks::GetGlobalTestHooks();

    if (globalTestHooks && globalTestHooks->AreExpressionAnimationStatusNotificationsRaised())
    {
        globalTestHooks->NotifyExpressionAnimationStatusChanged(*this, isExpressionAnimationStarted, propertyName);
    }
}

void ScrollPresenter::RaiseExtentChanged()
{
    if (m_extentChangedEventSource)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        m_extentChangedEventSource(*this, nullptr);
    }
}

void ScrollPresenter::RaiseStateChanged()
{
    if (m_stateChangedEventSource)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        m_stateChangedEventSource(*this, nullptr);
    }
}

void ScrollPresenter::RaiseScrollStarting(
    int32_t offsetsChangeCorrelationId,
    double anticipatedHorizontalOffset,
    double anticipatedVerticalOffset,
    float anticipatedZoomFactor)
{
    if (m_scrollStartingEventSource)
    {
        auto scrollStartingEventArgs = winrt::make_self<ScrollingScrollStartingEventArgs>();

        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_INT, METH_NAME, this, offsetsChangeCorrelationId);
        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, anticipatedHorizontalOffset, anticipatedVerticalOffset);
        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_FLT, METH_NAME, this, anticipatedZoomFactor);

        scrollStartingEventArgs->SetCorrelationId(offsetsChangeCorrelationId);
        scrollStartingEventArgs->SetHorizontalOffset(anticipatedHorizontalOffset);
        scrollStartingEventArgs->SetVerticalOffset(anticipatedVerticalOffset);
        scrollStartingEventArgs->SetZoomFactor(anticipatedZoomFactor);
        m_scrollStartingEventSource(*this, *scrollStartingEventArgs);
    }
}

void ScrollPresenter::RaiseZoomStarting(
    int32_t zoomFactorChangeCorrelationId,
    double anticipatedHorizontalOffset,
    double anticipatedVerticalOffset,
    float anticipatedZoomFactor)
{
    if (m_zoomStartingEventSource)
    {
        auto zoomStartingEventArgs = winrt::make_self<ScrollingZoomStartingEventArgs>();

        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_INT, METH_NAME, this, zoomFactorChangeCorrelationId);
        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, anticipatedHorizontalOffset, anticipatedVerticalOffset);
        SCROLLPRESENTER_TRACE_INFO_DBG(*this, TRACE_MSG_METH_FLT, METH_NAME, this, anticipatedZoomFactor);

        zoomStartingEventArgs->SetCorrelationId(zoomFactorChangeCorrelationId);
        zoomStartingEventArgs->SetHorizontalOffset(anticipatedHorizontalOffset);
        zoomStartingEventArgs->SetVerticalOffset(anticipatedVerticalOffset);
        zoomStartingEventArgs->SetZoomFactor(anticipatedZoomFactor);
        m_zoomStartingEventSource(*this, *zoomStartingEventArgs);
    }
}

void ScrollPresenter::RaiseViewChanged()
{
    if (m_viewChangedEventSource)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewChangedEventSource(*this, nullptr);
    }

    InvalidateViewport();
}

winrt::CompositionAnimation ScrollPresenter::RaiseScrollAnimationStarting(
    const winrt::Vector3KeyFrameAnimation& positionAnimation,
    const winrt::float2& startPosition,
    const winrt::float2& endPosition,
    int32_t offsetsChangeCorrelationId)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, offsetsChangeCorrelationId);
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_FLT_FLT_FLT_FLT, METH_NAME, this,
        startPosition.x, startPosition.y,
        endPosition.x, endPosition.y);

    if (m_scrollAnimationStartingEventSource)
    {
        auto scrollAnimationStartingEventArgs = winrt::make_self<ScrollingScrollAnimationStartingEventArgs>();

        if (offsetsChangeCorrelationId != s_noOpCorrelationId)
        {
            scrollAnimationStartingEventArgs->SetOffsetsChangeCorrelationId(offsetsChangeCorrelationId);
        }

        scrollAnimationStartingEventArgs->SetAnimation(positionAnimation);
        scrollAnimationStartingEventArgs->SetStartPosition(startPosition);
        scrollAnimationStartingEventArgs->SetEndPosition(endPosition);
        m_scrollAnimationStartingEventSource(*this, *scrollAnimationStartingEventArgs);
        return scrollAnimationStartingEventArgs->GetAnimation();
    }
    else
    {
        return positionAnimation;
    }
}

winrt::CompositionAnimation ScrollPresenter::RaiseZoomAnimationStarting(
    const winrt::ScalarKeyFrameAnimation& zoomFactorAnimation,
    const float endZoomFactor,
    const winrt::float2& centerPoint,
    int32_t zoomFactorChangeCorrelationId)
{
    SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_FLT_FLT_STR_INT, METH_NAME, this,
        m_zoomFactor,
        endZoomFactor,
        TypeLogging::Float2ToString(centerPoint).c_str(),
        zoomFactorChangeCorrelationId);

    if (m_zoomAnimationStartingEventSource)
    {
        auto zoomAnimationStartingEventArgs = winrt::make_self<ScrollingZoomAnimationStartingEventArgs>();

        if (zoomFactorChangeCorrelationId != s_noOpCorrelationId)
        {
            zoomAnimationStartingEventArgs->SetZoomFactorChangeCorrelationId(zoomFactorChangeCorrelationId);
        }

        zoomAnimationStartingEventArgs->SetAnimation(zoomFactorAnimation);
        zoomAnimationStartingEventArgs->SetCenterPoint(centerPoint);
        zoomAnimationStartingEventArgs->SetStartZoomFactor(m_zoomFactor);
        zoomAnimationStartingEventArgs->SetEndZoomFactor(endZoomFactor);
        m_zoomAnimationStartingEventSource(*this, *zoomAnimationStartingEventArgs);
        return zoomAnimationStartingEventArgs->GetAnimation();
    }
    else
    {
        return zoomFactorAnimation;
    }
}

void ScrollPresenter::RaiseViewChangeCompleted(
    bool isForScroll,
    ScrollPresenterViewChangeResult result,
    int32_t viewChangeCorrelationId)
{
    if (viewChangeCorrelationId)
    {
        if (isForScroll && m_scrollCompletedEventSource)
        {
            SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
                TypeLogging::ScrollPresenterViewChangeResultToString(result).c_str(),
                viewChangeCorrelationId);

            auto scrollCompletedEventArgs = winrt::make_self<ScrollingScrollCompletedEventArgs>();

            scrollCompletedEventArgs->Result(result);
            scrollCompletedEventArgs->OffsetsChangeCorrelationId(viewChangeCorrelationId);
            m_scrollCompletedEventSource(*this, *scrollCompletedEventArgs);
        }
        else if (!isForScroll && m_zoomCompletedEventSource)
        {
            SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
                TypeLogging::ScrollPresenterViewChangeResultToString(result).c_str(),
                viewChangeCorrelationId);

            auto zoomCompletedEventArgs = winrt::make_self<ScrollingZoomCompletedEventArgs>();

            zoomCompletedEventArgs->Result(result);
            zoomCompletedEventArgs->ZoomFactorChangeCorrelationId(viewChangeCorrelationId);
            m_zoomCompletedEventSource(*this, *zoomCompletedEventArgs);
        }
    }

    InvalidateViewport();
}

// Returns False when ScrollingBringingIntoViewEventArgs.Cancel is set to True to skip the operation.
bool ScrollPresenter::RaiseBringingIntoView(
    double targetZoomedHorizontalOffset,
    double targetZoomedVerticalOffset,
    const winrt::BringIntoViewRequestedEventArgs& requestEventArgs,
    int32_t offsetsChangeCorrelationId,
    _Inout_ winrt::ScrollingSnapPointsMode* snapPointsMode)
{
    if (m_bringingIntoViewEventSource)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        auto bringingIntoViewEventArgs = winrt::make_self<ScrollingBringingIntoViewEventArgs>();

        bringingIntoViewEventArgs->SnapPointsMode(*snapPointsMode);
        bringingIntoViewEventArgs->OffsetsChangeCorrelationId(offsetsChangeCorrelationId);
        bringingIntoViewEventArgs->RequestEventArgs(requestEventArgs);
        bringingIntoViewEventArgs->TargetOffsets(targetZoomedHorizontalOffset, targetZoomedVerticalOffset);

        m_bringingIntoViewEventSource(*this, *bringingIntoViewEventArgs);
        *snapPointsMode = bringingIntoViewEventArgs->SnapPointsMode();
        return !bringingIntoViewEventArgs->Cancel();
    }
    return true;
}

#ifdef DBG
void ScrollPresenter::DumpMinMaxPositions()
{
    MUX_ASSERT(m_interactionTracker);

    const winrt::UIElement content = Content();

    if (!content)
    {
        // Min/MaxPosition == (0, 0)
        return;
    }

    const winrt::Visual scrollPresenterVisual = winrt::ElementCompositionPreview::GetElementVisual(*this);
    const winrt::FrameworkElement contentAsFE = content.try_as<winrt::FrameworkElement>();
    float minPosX = 0.0f;
    float minPosY = 0.0f;
    const float extentWidth = static_cast<float>(m_unzoomedExtentWidth);
    const float extentHeight = static_cast<float>(m_unzoomedExtentHeight);

    if (contentAsFE)
    {
        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            minPosX = std::min(0.0f, (extentWidth * m_interactionTracker.Scale() - scrollPresenterVisual.Size().x) / 2.0f);
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            minPosX = std::min(0.0f, extentWidth * m_interactionTracker.Scale() - scrollPresenterVisual.Size().x);
        }

        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            minPosY = std::min(0.0f, (extentHeight * m_interactionTracker.Scale() - scrollPresenterVisual.Size().y) / 2.0f);
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            minPosY = std::min(0.0f, extentHeight * m_interactionTracker.Scale() - scrollPresenterVisual.Size().y);
        }
    }

    float maxPosX = std::max(0.0f, extentWidth * m_interactionTracker.Scale() - scrollPresenterVisual.Size().x);
    float maxPosY = std::max(0.0f, extentHeight * m_interactionTracker.Scale() - scrollPresenterVisual.Size().y);

    if (contentAsFE)
    {
        if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Center ||
            contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
        {
            maxPosX = (extentWidth * m_interactionTracker.Scale() - scrollPresenterVisual.Size().x) >= 0 ?
                (extentWidth * m_interactionTracker.Scale() - scrollPresenterVisual.Size().x) : (extentWidth * m_interactionTracker.Scale() - scrollPresenterVisual.Size().x) / 2.0f;
        }
        else if (contentAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Right)
        {
            maxPosX = extentWidth * m_interactionTracker.Scale() - scrollPresenterVisual.Size().x;
        }

        if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Center ||
            contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
        {
            maxPosY = (extentHeight * m_interactionTracker.Scale() - scrollPresenterVisual.Size().y) >= 0 ?
                (extentHeight * m_interactionTracker.Scale() - scrollPresenterVisual.Size().y) : (extentHeight * m_interactionTracker.Scale() - scrollPresenterVisual.Size().y) / 2.0f;
        }
        else if (contentAsFE.VerticalAlignment() == winrt::VerticalAlignment::Bottom)
        {
            maxPosY = extentHeight * m_interactionTracker.Scale() - scrollPresenterVisual.Size().y;
        }
    }
}
#endif // DBG

#ifdef DBG

winrt::hstring ScrollPresenter::DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_ContentProperty)
    {
        return L"Content";
    }
    else if (dependencyProperty == s_BackgroundProperty)
    {
        return L"Background";
    }
    else if (dependencyProperty == s_ContentOrientationProperty)
    {
        return L"ContentOrientation";
    }
    else if (dependencyProperty == s_VerticalScrollChainModeProperty)
    {
        return L"VerticalScrollChainMode";
    }
    else if (dependencyProperty == s_ZoomChainModeProperty)
    {
        return L"ZoomChainMode";
    }
    else if (dependencyProperty == s_HorizontalScrollRailModeProperty)
    {
        return L"HorizontalScrollRailMode";
    }
    else if (dependencyProperty == s_VerticalScrollRailModeProperty)
    {
        return L"VerticalScrollRailMode";
    }
    else if (dependencyProperty == s_HorizontalScrollModeProperty)
    {
        return L"HorizontalScrollMode";
    }
    else if (dependencyProperty == s_VerticalScrollModeProperty)
    {
        return L"VerticalScrollMode";
    }
    else if (dependencyProperty == s_ComputedHorizontalScrollModeProperty)
    {
        return L"ComputedHorizontalScrollMode";
    }
    else if (dependencyProperty == s_ComputedVerticalScrollModeProperty)
    {
        return L"ComputedVerticalScrollMode";
    }
    else if (dependencyProperty == s_ZoomModeProperty)
    {
        return L"ZoomMode";
    }
    else if (dependencyProperty == s_IgnoredInputKindsProperty)
    {
        return L"IgnoredInputKinds";
    }
    else if (dependencyProperty == s_MinZoomFactorProperty)
    {
        return L"MinZoomFactor";
    }
    else if (dependencyProperty == s_MaxZoomFactorProperty)
    {
        return L"MaxZoomFactor";
    }
    else if (dependencyProperty == s_HorizontalAnchorRatioProperty)
    {
        return L"HorizontalAnchorRatio";
    }
    else if (dependencyProperty == s_VerticalAnchorRatioProperty)
    {
        return L"VerticalAnchorRatio";
    }
    else
    {
        return L"UNKNOWN";
    }
}

#endif
