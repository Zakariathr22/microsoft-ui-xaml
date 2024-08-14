﻿// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 17.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    using System;
    
    /// <summary>
    /// Class to produce the template output
    /// </summary>
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "17.0.0.0")]
    internal partial class CppWinRT_BindingInfoPass1 : CppWinRT_CodeGenerator<BindingInfoDefinition>
    {
        /// <summary>
        /// Create the template output
        /// </summary>
        public override string TransformText()
        {
            this.Write(@"//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------
#pragma once

#include <unknwn.h>

// Undefine GetCurrentTime macro to prevent
// conflict with Storyboard::GetCurrentTime
#undef GetCurrentTime

#include ""winrt/windows.foundation.h""
#include ""winrt/windows.ui.xaml.interop.h""
#include ""winrt/microsoft.ui.xaml.controls.h""
#include ""winrt/microsoft.ui.xaml.data.h""
#include ""winrt/microsoft.ui.xaml.markup.h""

namespace winrt::");
            this.Write(this.ToStringHelper.ToStringWithCulture(Colonize(ProjectInfo.RootNamespace)));
            this.Write("::implementation\r\n{\r\n    using DataContextChangedEventArgs = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.Xaml)));
            this.Write("::DataContextChangedEventArgs;\r\n    using DependencyObject = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.Xaml)));
            this.Write("::DependencyObject;\r\n    using DependencyProperty = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.Xaml)));
            this.Write("::DependencyProperty;\r\n    using FrameworkElement = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.Xaml)));
            this.Write("::FrameworkElement;\r\n    using IInspectable = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.WindowsFoundation)));
            this.Write("::IInspectable;\r\n    using INotifyCollectionChanged = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlInterop)));
            this.Write("::INotifyCollectionChanged;\r\n    using INotifyPropertyChanged = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlData)));
            this.Write("::INotifyPropertyChanged;\r\n    using PropertyChangedEventArgs = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlData)));
            this.Write("::PropertyChangedEventArgs;\r\n    using NotifyCollectionChangedEventArgs = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlInterop)));
            this.Write("::NotifyCollectionChangedEventArgs;\r\n    using ContainerContentChangingEventArgs " +
                    "= ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlControls)));
            this.Write("::ContainerContentChangingEventArgs;\r\n    using IComponentConnector = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlMarkup)));
            this.Write("::IComponentConnector;\r\n    using WindowActivatedEventArgs = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.Xaml)));
            this.Write("::WindowActivatedEventArgs;\r\n");
 if (ProjectInfo.IsInputValidationEnabled) {
            this.Write("    using INotifyDataErrorInfo = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlData)));
            this.Write("::INotifyDataErrorInfo;\r\n    using DataErrorsChangedEventArgs = ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlData)));
            this.Write("::DataErrorsChangedEventArgs;\r\n");
 }
            this.Write(@"
    struct XamlBindings;

    struct IXamlBindings
    {
        virtual ~IXamlBindings() {};
        virtual bool IsInitialized() = 0;
        virtual void Update() = 0;
        virtual bool SetDataRoot(IInspectable const& data) = 0;
        virtual void StopTracking() = 0;
        virtual void Connect(int connectionId, IInspectable const& target) = 0;
        virtual void Recycle() = 0;
        virtual void ProcessBindings(IInspectable const& item, int itemIndex, int phase, int32_t& nextPhase) = 0;
        virtual void SubscribeForDataContextChanged(FrameworkElement const& object, XamlBindings& handler) = 0;
        virtual void DisconnectUnloadedObject(int connectionId) = 0;
    };

    struct IXamlBindingTracking
    {
        virtual void PropertyChanged(IInspectable const& sender, PropertyChangedEventArgs const& e) = 0;
        virtual void CollectionChanged(IInspectable const& sender, NotifyCollectionChangedEventArgs const& e) = 0;
        virtual void DependencyPropertyChanged(DependencyObject const& sender, DependencyProperty const& prop) = 0;
        virtual void VectorChanged(IInspectable const& sender, ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.WindowsFoundationCollections)));
            this.Write("::IVectorChangedEventArgs const& e) = 0;\r\n        virtual void MapChanged(IInspec" +
                    "table const& sender, ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.WindowsFoundationCollections)));
            this.Write("::IMapChangedEventArgs<::winrt::hstring> const& e) = 0;\r\n");
 if (ProjectInfo.IsInputValidationEnabled) {
            this.Write("        virtual void ErrorsChanged(IInspectable const& sender, DataErrorsChangedE" +
                    "ventArgs const& e) = 0;\r\n");
 } 
            this.Write("    };\r\n\r\n    struct XamlBindings : public winrt::implements<XamlBindings,\r\n     " +
                    "   ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.Xaml)));
            this.Write("::IDataTemplateExtension,\r\n        ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlMarkup)));
            this.Write("::IComponentConnector,\r\n        ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlMarkup)));
            this.Write("::IDataTemplateComponent>\r\n    {\r\n        XamlBindings(std::shared_ptr<IXamlBindi" +
                    "ngs>&& pBindings);\r\n\r\n        // IComponentConnector\r\n        void Connect(int c" +
                    "onnectionId, IInspectable const& target);\r\n        IComponentConnector GetBindin" +
                    "gConnector(int32_t, IInspectable const&);\r\n\r\n        // IDataTemplateComponent\r\n" +
                    "        virtual void ProcessBindings(IInspectable const& item, int itemIndex, in" +
                    "t phase, int32_t& nextPhase);\r\n        virtual void Recycle();\r\n\r\n        // IDa" +
                    "taTemplateExtension\r\n        bool ProcessBinding(uint32_t phase);\r\n        int P" +
                    "rocessBindings(ContainerContentChangingEventArgs const& args);\r\n        void Res" +
                    "etTemplate();\r\n\r\n        void Initialize();\r\n        void Update();\r\n        voi" +
                    "d StopTracking();\r\n        void Loading(FrameworkElement const& src, IInspectabl" +
                    "e const& data);\r\n        void Activated(IInspectable const& sender, WindowActiva" +
                    "tedEventArgs const& args);\r\n        void DataContextChanged(FrameworkElement con" +
                    "st& sender, DataContextChangedEventArgs const& args);\r\n        void SubscribeFor" +
                    "DataContextChanged(FrameworkElement const& object);\r\n        virtual void Discon" +
                    "nectUnloadedObject(int connectionId);\r\n\r\n    private:\r\n        std::shared_ptr<I" +
                    "XamlBindings> _pBindings;\r\n    };\r\n\r\n    template <typename TBindingsTracking>\r\n" +
                    "    struct XamlBindingsBase : public IXamlBindings\r\n    {\r\n    protected:\r\n     " +
                    "   bool _isInitialized = false;\r\n        std::shared_ptr<TBindingsTracking> _bin" +
                    "dingsTracking;\r\n        winrt::event_token _dataContextChangedToken {};\r\n       " +
                    " static const int NOT_PHASED = (1 << 31);\r\n        static const int DATA_CHANGED" +
                    " = (1 << 30);\r\n\r\n    protected:\r\n        XamlBindingsBase() = default;\r\n\r\n      " +
                    "  virtual ~XamlBindingsBase()\r\n        {\r\n            if (_bindingsTracking)\r\n  " +
                    "          {\r\n                _bindingsTracking->SetListener(nullptr);\r\n         " +
                    "       _bindingsTracking.reset();\r\n            }\r\n        }\r\n\r\n        virtual v" +
                    "oid ReleaseAllListeners()\r\n        {\r\n            // Overridden in the binding c" +
                    "lass as needed.\r\n        }\r\n\r\n    public:\r\n        void InitializeTracking(IXaml" +
                    "BindingTracking* pBindingsTracking)\r\n        {\r\n            _bindingsTracking = " +
                    "std::make_shared<TBindingsTracking>();\r\n            _bindingsTracking->SetListen" +
                    "er(pBindingsTracking);\r\n        }\r\n\r\n        virtual void StopTracking() overrid" +
                    "e\r\n        {\r\n            ReleaseAllListeners();\r\n            this->_isInitializ" +
                    "ed = false;\r\n        }\r\n\r\n        virtual bool IsInitialized() override\r\n       " +
                    " {\r\n            return this->_isInitialized;\r\n        }\r\n\r\n        void Subscrib" +
                    "eForDataContextChanged(FrameworkElement const& object, XamlBindings& handler) ov" +
                    "erride\r\n        {\r\n            this->_dataContextChangedToken = object.DataConte" +
                    "xtChanged({ &handler, &XamlBindings::DataContextChanged });\r\n        }\r\n\r\n      " +
                    "  virtual void Recycle() override\r\n        {\r\n            // Overridden in the b" +
                    "inding class as needed.\r\n        }\r\n\r\n        virtual void ProcessBindings(IInsp" +
                    "ectable const&, int, int, int32_t& nextPhase) override\r\n        {\r\n            /" +
                    "/ Overridden in the binding class as needed.\r\n            nextPhase = -1;\r\n     " +
                    "   }\r\n    };\r\n\r\n    struct XamlBindingTrackingBase\r\n    {\r\n        XamlBindingTr" +
                    "ackingBase();\r\n        void SetListener(IXamlBindingTracking* pBindings);\r\n     " +
                    "   \r\n        // Event handlers\r\n        void PropertyChanged(IInspectable const&" +
                    " sender, PropertyChangedEventArgs const& e);\r\n        void CollectionChanged(IIn" +
                    "spectable const& sender, NotifyCollectionChangedEventArgs const& e);\r\n        vo" +
                    "id DependencyPropertyChanged(DependencyObject const& sender, DependencyProperty " +
                    "const& prop);\r\n        void VectorChanged(IInspectable const& sender, ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.WindowsFoundationCollections)));
            this.Write("::IVectorChangedEventArgs const& e);\r\n        void MapChanged(IInspectable const&" +
                    " sender, ");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.WindowsFoundationCollections)));
            this.Write("::IMapChangedEventArgs<::winrt::hstring> const& e);\r\n");
 if (ProjectInfo.IsInputValidationEnabled) {
            this.Write("        void ErrorsChanged(IInspectable const& sender, DataErrorsChangedEventArgs" +
                    " const& e);\r\n");
 } 
            this.Write(@"
        // Listener update functions
        void UpdatePropertyChangedListener(INotifyPropertyChanged const& obj, INotifyPropertyChanged& cache, ::winrt::event_token& token);
        void UpdatePropertyChangedListener(INotifyPropertyChanged const& obj, ::winrt::weak_ref<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlData)));
            this.Write(@"::INotifyPropertyChanged>& cache, ::winrt::event_token& token);
        void UpdateCollectionChangedListener(INotifyCollectionChanged const& obj, INotifyCollectionChanged& cache, ::winrt::event_token& token);
        void UpdateDependencyPropertyChangedListener(DependencyObject const& obj, DependencyProperty const& property, DependencyObject&  cache, int64_t& token);
        void UpdateDependencyPropertyChangedListener(DependencyObject const& obj, DependencyProperty const& property, ::winrt::weak_ref<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.Xaml)));
            this.Write("::DependencyObject>& cache, int64_t& token);\r\n");
 if (ProjectInfo.IsInputValidationEnabled) {
            this.Write("        void UpdateErrorsChangedListener(INotifyDataErrorInfo const& obj, INotify" +
                    "DataErrorInfo& cache, ::winrt::event_token& token);\r\n        void UpdateErrorsCh" +
                    "angedListener(INotifyDataErrorInfo const& obj, ::winrt::weak_ref<");
            this.Write(this.ToStringHelper.ToStringWithCulture(Projection(KnownNamespaces.XamlData)));
            this.Write("::INotifyDataErrorInfo>& cache, ::winrt::event_token& token);\r\n");
 } 
            this.Write("\r\n    private:\r\n        IXamlBindingTracking* _pBindingsTrackingWeakRef{nullptr};" +
                    "\r\n    };\r\n\r\n    template <typename T>\r\n    struct ResolveHelper\r\n    {\r\n        " +
                    "static T Resolve(::winrt::weak_ref<T> const& wr)\r\n        {\r\n            return " +
                    "wr.get();\r\n        }\r\n    };\r\n\r\n    template<>\r\n    struct ResolveHelper<::winrt" +
                    "::hstring>\r\n    {\r\n        using ResolveType = ::winrt::Windows::Foundation::IRe" +
                    "ference<::winrt::hstring>;\r\n        static ::winrt::hstring Resolve(::winrt::wea" +
                    "k_ref<ResolveType> const& wr)\r\n        {\r\n            return wr.get().Value();\r\n" +
                    "        }\r\n    };\r\n\r\n    template<typename T, typename TBindingsTracking> \r\n    " +
                    "struct ReferenceTypeXamlBindings : public XamlBindingsBase<TBindingsTracking>\r\n " +
                    "   {\r\n    protected:\r\n        ReferenceTypeXamlBindings() {}\r\n\r\n        virtual " +
                    "void Update_(T, int)\r\n        {\r\n            // Overridden in the binding class " +
                    "as needed.\r\n        }\r\n\r\n    public:\r\n        T GetDataRoot()\r\n        {\r\n      " +
                    "      return ResolveHelper<T>::Resolve(this->_dataRoot);\r\n        }\r\n\r\n        b" +
                    "ool SetDataRoot(IInspectable const& data) override\r\n        {\r\n            if (d" +
                    "ata)\r\n            {\r\n                this->_dataRoot = ::winrt::unbox_value<T>(d" +
                    "ata);\r\n                return true;\r\n            }\r\n            return false;\r\n " +
                    "       }\r\n\r\n        virtual void Update() override\r\n        {\r\n            this-" +
                    ">Update_(this->GetDataRoot(), this->NOT_PHASED);\r\n            this->_isInitializ" +
                    "ed = true;\r\n        }\r\n\r\n    private:\r\n         ::winrt::weak_ref<T> _dataRoot;\r" +
                    "\n    };\r\n\r\n    template<typename T, typename TBindingsTracking> \r\n    struct Val" +
                    "ueTypeXamlBindings : public XamlBindingsBase<TBindingsTracking>\r\n    {\r\n    prot" +
                    "ected:\r\n        ValueTypeXamlBindings() {}\r\n\r\n        virtual void Update_(T, in" +
                    "t)\r\n        {\r\n            // Overridden in the binding class as needed.\r\n      " +
                    "  }\r\n\r\n    public:\r\n        T GetDataRoot()\r\n        {\r\n            return this-" +
                    ">_dataRoot;\r\n        }\r\n\r\n        bool SetDataRoot(IInspectable const& data) ove" +
                    "rride\r\n        {\r\n            if (data)\r\n            {\r\n                this->_d" +
                    "ataRoot = ::winrt::unbox_value<T>(data);\r\n                return true;\r\n        " +
                    "    }\r\n            return false;\r\n        }\r\n\r\n        virtual void Update() ove" +
                    "rride\r\n        {\r\n            this->Update_(this->GetDataRoot(), this->NOT_PHASE" +
                    "D);\r\n            this->_isInitialized = true;\r\n        }\r\n\r\n    private:\r\n      " +
                    "  T _dataRoot;\r\n    };\r\n}");
            return this.GenerationEnvironment.ToString();
        }
    }
}
