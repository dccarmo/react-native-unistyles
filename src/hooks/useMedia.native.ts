import { useEffect, useLayoutEffect, useState } from 'react'
import { StyleSheet, UnistyleDependency, UnistylesRuntime, type UnistylesStyleSheet } from '../specs'
import { isValidMq, parseMq, isUnistylesMq } from '../mq'

export const useMedia = (config: { mq: symbol }) => {
    const computeIsVisible = (): boolean => {
        const maybeMq = config.mq as unknown as string

        if (!isUnistylesMq(maybeMq)) {
            console.error(`🦄 Unistyles: Received invalid mq: ${maybeMq}`)

            return false
        }

        const parsedMq = parseMq(maybeMq)

        if (!isValidMq(parsedMq)) {
            console.error(`🦄 Unistyles: Received invalid mq where min is greater than max: ${maybeMq}`)

            return false
        }

        const { width, height } = UnistylesRuntime.screen

        if (parsedMq.minWidth !== undefined && width < parsedMq.minWidth) {
            return false
        }

        if (parsedMq.maxWidth !== undefined && width > parsedMq.maxWidth) {
            return false
        }

        if (parsedMq.minHeight !== undefined && height < parsedMq.minHeight) {
            return false
        }

        if (parsedMq.maxHeight !== undefined && height > parsedMq.maxHeight) {
            return false
        }

        return true
    }
    const [isVisible, setIsVisible] = useState<boolean | null>(computeIsVisible())

    useEffect(() => {
        setIsVisible(computeIsVisible())
    }, [config.mq])

    useLayoutEffect(() => {
        const removeChangeListener = (StyleSheet as UnistylesStyleSheet).addChangeListener(dependencies => {
            if (dependencies.includes(UnistyleDependency.Breakpoints)) {
                setIsVisible(computeIsVisible())
            }
        })

        return () => {
            removeChangeListener()
        }
    }, [config.mq])

    return {
        isVisible
    }
}
