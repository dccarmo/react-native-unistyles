import type { UnistylesBreakpoints } from './global'
import { UnistylesRuntime } from './specs'
import type { Nullable } from './types'

type MQValue = keyof UnistylesBreakpoints | number

type MQHandler = {
    only: {
        width(wMin?: Nullable<MQValue>, wMax?: MQValue): symbol,
        height(hMin?: Nullable<MQValue>, hMax?: MQValue): symbol,
    },
    width(wMin?: Nullable<MQValue>, wMax?: MQValue): {
        and: {
            height(hMin?: Nullable<MQValue>, hMax?: MQValue): symbol
        }
    },
    height(hMin?: Nullable<MQValue>, hMax?: MQValue): {
        and: {
            width(wMin?: Nullable<MQValue>, wMax?: MQValue): symbol
        }
    }
}

const getMQValue = (value: Nullable<MQValue>) => {
    if (typeof value === 'number') {
        return value
    }

    if (value === null) {
        return 0
    }

    const breakpoints = UnistylesRuntime.breakpoints

    return breakpoints[value] ?? 0
}

/**
 * Utility to create cross-platform media queries
 * @returns - JavaScript symbol to be used in your stylesheet
 */
export const mq: MQHandler = {
    only: {
        width: (wMin: Nullable<MQValue> = 0, wMax: MQValue = Number.POSITIVE_INFINITY) => (`:w[${getMQValue(wMin)}, ${getMQValue(wMax)}]` as unknown as symbol),
        height: (hMin: Nullable<MQValue> = 0, hMax: MQValue = Number.POSITIVE_INFINITY) => (`:h[${getMQValue(hMin)}, ${getMQValue(hMax)}]` as unknown as symbol)
    },
    width: (wMin: Nullable<MQValue> = 0, wMax: MQValue = Number.POSITIVE_INFINITY) => ({
        and: {
            height: (hMin: Nullable<MQValue> = 0, hMax: MQValue = Number.POSITIVE_INFINITY) =>
                (`:w[${getMQValue(wMin)}, ${getMQValue(wMax)}]:h[${getMQValue(hMin)}, ${getMQValue(hMax)}]` as unknown as symbol)
        }
    }),
    height: (hMin: Nullable<MQValue> = 0, hMax: MQValue = Number.POSITIVE_INFINITY) => ({
        and: {
            width: (wMin: Nullable<MQValue> = 0, wMax: MQValue = Number.POSITIVE_INFINITY) =>
                (`:w[${getMQValue(wMin)}, ${getMQValue(wMax)}]:h[${getMQValue(hMin)}, ${getMQValue(hMax)}]` as unknown as symbol)
        }
    })
}

