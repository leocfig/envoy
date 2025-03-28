#pragma once

#include "envoy/extensions/filters/network/sni_dynamic_forward_proxy/v3/sni_dynamic_forward_proxy.pb.h"
#include "envoy/network/filter.h"
#include "envoy/upstream/cluster_manager.h"

#include "source/common/common/logger.h"
#include "source/common/upstream/upstream_impl.h"
#include "source/extensions/common/dynamic_forward_proxy/dns_cache.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SniDynamicForwardProxy {

using FilterConfig =
    envoy::extensions::filters::network::sni_dynamic_forward_proxy::v3::FilterConfig;

class ProxyFilterConfig {
public:
  ProxyFilterConfig(
      const FilterConfig& proto_config,
      Extensions::Common::DynamicForwardProxy::DnsCacheManagerFactory& cache_manager_factory,
      Upstream::ClusterManager& cluster_manager, absl::Status& creation_status);

  Extensions::Common::DynamicForwardProxy::DnsCache& cache() { return *dns_cache_; }
  uint32_t port() { return port_; }
  bool saveUpstreamAddress() const { return save_upstream_address_; };

private:
  const uint32_t port_;
  const Extensions::Common::DynamicForwardProxy::DnsCacheManagerSharedPtr dns_cache_manager_;
  Extensions::Common::DynamicForwardProxy::DnsCacheSharedPtr dns_cache_;
  const bool save_upstream_address_;
};

using ProxyFilterConfigSharedPtr = std::shared_ptr<ProxyFilterConfig>;

class ProxyFilter
    : public Network::ReadFilter,
      public Extensions::Common::DynamicForwardProxy::DnsCache::LoadDnsCacheEntryCallbacks,
      Logger::Loggable<Logger::Id::forward_proxy> {
public:
  ProxyFilter(ProxyFilterConfigSharedPtr config);

  // Network::ReadFilter
  Network::FilterStatus onData(Buffer::Instance&, bool) override {
    return Network::FilterStatus::Continue;
  }
  Network::FilterStatus onNewConnection() override;
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override {
    read_callbacks_ = &callbacks;
  }

  // Extensions::Common::DynamicForwardProxy::DnsCache::LoadDnsCacheEntryCallbacks
  void onLoadDnsCacheComplete(
      const Extensions::Common::DynamicForwardProxy::DnsHostInfoSharedPtr&) override;

private:
  void addHostAddressToFilterState(const Network::Address::InstanceConstSharedPtr& address);

  const ProxyFilterConfigSharedPtr config_;
  Upstream::ResourceAutoIncDecPtr circuit_breaker_;
  Extensions::Common::DynamicForwardProxy::DnsCache::LoadDnsCacheEntryHandlePtr cache_load_handle_;
  Network::ReadFilterCallbacks* read_callbacks_{};
};

} // namespace SniDynamicForwardProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
