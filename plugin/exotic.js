function handleExoticImport(t, path, state, exoticImport) {
    const specifiers = path.node.specifiers
    const source = path.node.source

    if (path.node.importKind !== 'value') {
        return
    }

    specifiers.forEach(specifier => {
        for (const rule of exoticImport.imports) {
            const hasMatchingImportType = (!rule.isDefault && t.isImportSpecifier(specifier)) || (rule.isDefault && t.isImportDefaultSpecifier(specifier))
            const hasMatchingImportName = rule.isDefault || (!rule.isDefault && rule.name === specifier.local.name)
            const hasMatchingPath = rule.path === source.value

            if (!hasMatchingImportType || !hasMatchingImportName || !hasMatchingPath) {
                continue
            }

            if (t.isImportDefaultSpecifier(specifier)) {
                const newImport = t.importDeclaration(
                    [t.importDefaultSpecifier(t.identifier(specifier.local.name))],
                    t.stringLiteral(state.opts.isLocal
                        ? state.file.opts.filename.split('react-native-unistyles').at(0).concat(`react-native-unistyles/components/native/${rule.mapTo}`)
                        : `react-native-unistyles/components/native/${rule.mapTo}`
                    )
                )

                path.replaceWith(newImport)
            } else {
                const newImport = t.importDeclaration(
                    [t.importSpecifier(t.identifier(rule.mapTo), t.identifier(rule.mapTo))],
                    t.stringLiteral(state.opts.isLocal
                        ? state.file.opts.filename.split('react-native-unistyles').at(0).concat(`react-native-unistyles/components/native/${rule.mapTo}`)
                        : `react-native-unistyles/components/native/${rule.mapTo}`
                    )
                )

                path.node.specifiers = specifiers.filter(s => s !== specifier)

                if (path.node.specifiers.length === 0) {
                    path.replaceWith(newImport)
                } else {
                    path.insertBefore(newImport)
                }
            }

            return
        }
    })
}

module.exports = {
    handleExoticImport
}
