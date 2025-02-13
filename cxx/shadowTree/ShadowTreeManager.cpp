#include "ShadowTreeManager.h"

using namespace margelo::nitro::unistyles;
using namespace facebook::react;
using namespace facebook;

using AffectedNodes = std::unordered_map<const ShadowNodeFamily*, std::unordered_set<int>>;

void shadow::ShadowTreeManager::updateShadowTree(const ShadowTreeRegistry& shadowTreeRegistry) {
    auto& registry = core::UnistylesRegistry::get();
    
    registry.trafficController.withLock([&](){
        auto updates = registry.trafficController.getUpdates();

        if (updates.empty()) {
            return;
        }

        shadowTreeRegistry.enumerate([&updates](const ShadowTree& shadowTree, bool& stop){
            // we could iterate via updates and create multiple commits
            // but it can cause performance issues for hundreds of nodes
            // so let's mutate Shadow Tree in single transaction
            auto transaction = [&](const RootShadowNode& oldRootShadowNode) {
                auto affectedNodes = shadow::ShadowTreeManager::findAffectedNodes(oldRootShadowNode, updates);
                auto newRootNode = std::static_pointer_cast<RootShadowNode>(shadow::ShadowTreeManager::cloneShadowTree(
                    oldRootShadowNode,
                    updates,
                    affectedNodes
                ));

                // set unistyles trait
                auto unistylesRootNode = std::reinterpret_pointer_cast<core::UnistylesCommitShadowNode>(newRootNode);

                unistylesRootNode->addUnistylesCommitTrait();

                return newRootNode;
            };

            // commit once!
            // CommitOptions:
            // enableStateReconciliation: https://reactnative.dev/architecture/render-pipeline#react-native-renderer-state-updates
            // mountSynchronously: must be true as this is update from C++ not React
            shadowTree.commit(transaction, {false, true});


            // for now we're assuming single surface, can be improved in the future
            // stop = true means stop enumerating next shadow tree
            // so in other words first shadow tree is our desired tree
            stop = true;
        });
    });
}

// based on Reanimated algorithm
// For each affected family we're gathering affected nodes (their indexes)
// Example:
//      A
//    /   \
//   B     C
//  / \
// D   E*
//    / \
//   F   G
//
// For ShadowFamily E* we will get:
//[
//  0 - because B is a first children of A,
//  1 - because E is a second children of B
//]
// A, B and E are affected now
AffectedNodes shadow::ShadowTreeManager::findAffectedNodes(const RootShadowNode& rootNode, ShadowLeafUpdates& updates) {
    AffectedNodes affectedNodes;

    // compute affected nodes (sub tree)
    std::for_each(updates.begin(), updates.end(), [&](const auto& pair) {
        const auto& [family, _] = pair;
        const auto familyAncestors = family->getAncestors(rootNode);

        for (const auto& [parentNode, index] : std::ranges::reverse_view(familyAncestors)) {
            const auto parentFamily = &parentNode.get().getFamily();
            std::unordered_set<int>& affectedNode = affectedNodes[parentFamily];

            affectedNode.insert(index);
        }
    });

    return affectedNodes;
}

// based on Reanimated algorithm
// clone affected nodes recursively, inject props and commit tree
ShadowNode::Unshared shadow::ShadowTreeManager::cloneShadowTree(const ShadowNode &shadowNode, ShadowLeafUpdates& updates, AffectedNodes& affectedNodes) {
    const auto family = &shadowNode.getFamily();
    const auto rawPropsIt = updates.find(family);
    const auto childrenIt = affectedNodes.find(family);
    auto children = shadowNode.getChildren();

    // for each affected node
    if (childrenIt != affectedNodes.end()) {
        // get all indexes of children and clone it recursively
        for (const auto index : childrenIt->second) {
            children[index] = cloneShadowTree(*children[index], updates, affectedNodes);
        }
    }

    Props::Shared updatedProps = nullptr;

    // clone props for our target shadow node and place fresh RawProps
    if (rawPropsIt != updates.end()) {
        PropsParserContext propsParserContext{
            shadowNode.getSurfaceId(),
            *shadowNode.getContextContainer()
        };

        // this is important and critical
        // first of all Android doesn't like nullish props (they work perfectly fine on iOS)
        // second of all Android props MUST be constructed from previous props, otherwise RawProps::~RawProps error occurs
        // Meta wants to remove shadowNode.getProps()->rawProps, but for now it's the only viable solution
        #ifdef ANDROID
            auto safeProps = rawPropsIt->second == nullptr ? folly::dynamic::object() : rawPropsIt->second;
            auto newProps = folly::dynamic::merge(shadowNode.getProps()->rawProps, safeProps);
        #else
            auto newProps = rawPropsIt->second;
        #endif

        updatedProps = shadowNode
            .getComponentDescriptor()
            .cloneProps(propsParserContext, shadowNode.getProps(), RawProps(newProps));
    }

    return shadowNode.clone({
        updatedProps ? updatedProps : ShadowNodeFragment::propsPlaceholder(),
        std::make_shared<ShadowNode::ListOfShared>(children),
        shadowNode.getState()
    });
}
