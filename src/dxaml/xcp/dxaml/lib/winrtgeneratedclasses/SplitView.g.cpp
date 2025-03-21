// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "SplitView.g.h"
#include "Brush.g.h"
#include "SplitViewTemplateSettings.g.h"
#include "UIElement.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::SplitViewGenerated::SplitViewGenerated()
{
}

DirectUI::SplitViewGenerated::~SplitViewGenerated()
{
}

HRESULT DirectUI::SplitViewGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::SplitView)))
    {
        *ppObject = static_cast<DirectUI::SplitView*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISplitView)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ISplitView*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::Internal::FrameworkUdk::IBackButtonPressedListener)))
    {
        *ppObject = static_cast<ABI::Microsoft::Internal::FrameworkUdk::IBackButtonPressedListener*>(this);
    }
    else
    {
        RRETURN(DirectUI::Control::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_CompactPaneLength(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_CompactPaneLength, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_CompactPaneLength(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_CompactPaneLength, value));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_Content(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_Content, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_Content(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_Content, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_DisplayMode(_Out_ ABI::Microsoft::UI::Xaml::Controls::SplitViewDisplayMode* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_DisplayMode, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_DisplayMode(ABI::Microsoft::UI::Xaml::Controls::SplitViewDisplayMode value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_DisplayMode, value));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_IsPaneOpen(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_IsPaneOpen, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_IsPaneOpen(BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_IsPaneOpen, value));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_LightDismissOverlayMode(_Out_ ABI::Microsoft::UI::Xaml::Controls::LightDismissOverlayMode* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_LightDismissOverlayMode, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_LightDismissOverlayMode(ABI::Microsoft::UI::Xaml::Controls::LightDismissOverlayMode value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_LightDismissOverlayMode, value));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_OpenPaneLength(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_OpenPaneLength, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_OpenPaneLength(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_OpenPaneLength, value));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_Pane(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_Pane, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_Pane(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_Pane, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_PaneBackground(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IBrush** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_PaneBackground, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_PaneBackground(_In_opt_ ABI::Microsoft::UI::Xaml::Media::IBrush* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_PaneBackground, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_PanePlacement(_Out_ ABI::Microsoft::UI::Xaml::Controls::SplitViewPanePlacement* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_PanePlacement, pValue));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::put_PanePlacement(ABI::Microsoft::UI::Xaml::Controls::SplitViewPanePlacement value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SplitView_PanePlacement, value));
}
IFACEMETHODIMP DirectUI::SplitViewGenerated::get_TemplateSettings(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::Primitives::ISplitViewTemplateSettings** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SplitView_TemplateSettings, ppValue));
}

// Events.
_Check_return_ HRESULT DirectUI::SplitViewGenerated::GetPaneClosedEventSourceNoRef(_Outptr_ PaneClosedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::SplitView_PaneClosed, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<PaneClosedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::SplitView_PaneClosed, this, /* bUseEventManager */ true);
        IFC(StoreEventSource(KnownEventIndex::SplitView_PaneClosed, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SplitViewGenerated::add_PaneClosed(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    PaneClosedEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaneClosedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SplitViewGenerated::remove_PaneClosed(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    PaneClosedEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>* pValue = (ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaneClosedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::SplitView_PaneClosed));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::SplitViewGenerated::GetPaneClosingEventSourceNoRef(_Outptr_ PaneClosingEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::SplitView_PaneClosing, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<PaneClosingEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::SplitView_PaneClosing, this, /* bUseEventManager */ true);
        IFC(StoreEventSource(KnownEventIndex::SplitView_PaneClosing, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SplitViewGenerated::add_PaneClosing(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, ABI::Microsoft::UI::Xaml::Controls::SplitViewPaneClosingEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    PaneClosingEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaneClosingEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SplitViewGenerated::remove_PaneClosing(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    PaneClosingEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, ABI::Microsoft::UI::Xaml::Controls::SplitViewPaneClosingEventArgs*>* pValue = (ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, ABI::Microsoft::UI::Xaml::Controls::SplitViewPaneClosingEventArgs*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaneClosingEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::SplitView_PaneClosing));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::SplitViewGenerated::GetPaneOpenedEventSourceNoRef(_Outptr_ PaneOpenedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::SplitView_PaneOpened, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<PaneOpenedEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::SplitView_PaneOpened, this, /* bUseEventManager */ true);
        IFC(StoreEventSource(KnownEventIndex::SplitView_PaneOpened, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SplitViewGenerated::add_PaneOpened(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    PaneOpenedEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaneOpenedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SplitViewGenerated::remove_PaneOpened(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    PaneOpenedEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>* pValue = (ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaneOpenedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::SplitView_PaneOpened));
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::SplitViewGenerated::GetPaneOpeningEventSourceNoRef(_Outptr_ PaneOpeningEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::SplitView_PaneOpening, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<PaneOpeningEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::SplitView_PaneOpening, this, /* bUseEventManager */ true);
        IFC(StoreEventSource(KnownEventIndex::SplitView_PaneOpening, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SplitViewGenerated::add_PaneOpening(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    PaneOpeningEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaneOpeningEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::SplitViewGenerated::remove_PaneOpening(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    PaneOpeningEventSourceType* pEventSource = nullptr;
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>* pValue = (ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::SplitView*, IInspectable*>*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetPaneOpeningEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::SplitView_PaneOpening));
    }

Cleanup:
    RRETURN(hr);
}

// Methods.
IFACEMETHODIMP DirectUI::SplitViewGenerated::OnBackButtonPressed(_Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "SplitView_OnBackButtonPressed", 0);
    }
    ARG_VALIDRETURNPOINTER(pResult);
    *pResult={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<SplitView*>(this)->OnBackButtonPressedImpl(pResult));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "SplitView_OnBackButtonPressed", hr);
    }
    RRETURN(hr);
}

HRESULT DirectUI::SplitViewFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISplitViewFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ISplitViewFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISplitViewStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ISplitViewStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::SplitViewFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::ISplitView** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISplitView);
    const GUID metadataAPIGUID = MetadataAPI::GetClassInfoByIndex(GetTypeIndex())->GetGuid();
    const KnownTypeIndex typeIndex = GetTypeIndex();

    if(uuidofGUID != metadataAPIGUID)
    {
        XAML_FAIL_FAST();
    }
#endif

    // Can't just IFC(_RETURN) this because for some validate calls (those with multiple template parameters), the
    // preprocessor gets confused at the "," in the template type-list before the function's opening parenthesis.
    // So we'll use IFC_RETURN syntax with a local hr variable, kind of weirdly.
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithBetterAggregableCoreObjectActivationFactory(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.
IFACEMETHODIMP DirectUI::SplitViewFactory::get_ContentProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_Content, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_PaneProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_Pane, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_IsPaneOpenProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_IsPaneOpen, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_OpenPaneLengthProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_OpenPaneLength, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_CompactPaneLengthProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_CompactPaneLength, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_PanePlacementProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_PanePlacement, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_DisplayModeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_DisplayMode, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_TemplateSettingsProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_TemplateSettings, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_PaneBackgroundProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_PaneBackground, ppValue));
}
IFACEMETHODIMP DirectUI::SplitViewFactory::get_LightDismissOverlayModeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SplitView_LightDismissOverlayMode, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_SplitView()
    {
        RRETURN(ctl::ActivationFactoryCreator<SplitViewFactory>::CreateActivationFactory());
    }
}
