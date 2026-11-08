# tensor-order-core

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Standard-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/Build-CMake%203.25+-informational.svg)](CMakeLists.txt)
[![Zero Dependencies](https://img.shields.io/badge/Dependencies-Zero-brightgreen.svg)](#zero-dependências)
[![Checked 128-bit](https://img.shields.io/badge/Arithmetic-Checked%20128--bit-orange.svg)](#aritmética-verificada-de-128-bits)

> **Mecanismo de otimização combinatória exata em C++20 para encontrar a árvore binária de contração com custo mínimo comprovado de multiplicações escalares para redes de tensores arbitrárias, com aritmética verificada em 128 bits, desempate canônico determinístico e recibos auditáveis de execução.**

---

## Visão Geral

Em computações de redes de tensores—abrangendo simulação de circuitos quânticos, decomposições em tensor train, física da matéria condensada e somatórios de Einstein (`einsum`) multidimensionais—a ordem em que tensores pares são contraídos dita criticamente o consumo de tempo e memória. Uma sequência ingênua ou aleatória pode gerar intermediários cujos produtos de dimensões explodem o orçamento computacional em ordens de magnitude (por exemplo, exigindo $10^{15}$ operações em vez de $10^4$).

O `tensor-order-core` resolve este problema combinatório de forma **exata** para redes de até $N = 12$ tensores através de programação dinâmica (DP) acelerada por máscaras de bits sobre todas as 261.625 bipartições possíveis. Ele garante:
- **Otimalidade Global Comprovada**: Encontra o mínimo absoluto de multiplicações aritméticas.
- **Aritmética Verificada de 128 Bits**: Interrompe e detecta overflow aritmético quando o volume de tensores intermediários excede $2^{64}-1$.
- **Zero Dependências Externas**: Utiliza exclusivamente a biblioteca padrão ISO C++20.
- **Recibos de Replay Autoauditáveis**: Produz um manifesto em pós-ordem que pode ser auditado e verificado de forma independente em tempo linear.
- **Determinismo Canônico**: Desempate lexicográfico estrito e reproduzível entre diferentes compiladores, threads e arquiteturas.

---

## Recursos Principais

1. **Programação Dinâmica Exata**: Avalia todos os $2^N$ subconjuntos e bipartições canônicas $A < B$ em menos de 5 milissegundos para redes de 12 tensores.
2. **Rastreador de Fronteira por Máscara de Bits**: Cálculo instantâneo dos eixos ativos dos intermediários via tabelas de frequência de grau.
3. **Aritmética Checada de 128 Bits (`UInt128`)**: Multiplicações e adições com detecção ativa de overflow durante o produto de dimensões.
4. **Poda e Limites de Segurança**:
   - `maxAllowedCost`: Encerra antecipadamente (`ProvenAboveCeiling`) caso o custo mínimo comprovado supere o teto estipulado.
   - `maxSplitsBudget`: Evita esgotamento de CPU em ambientes de consultas concorrentes.
5. **Recibos Auditáveis de Execução**: Gera um cronograma estruturado em pós-ordem com verificação passo a passo da álgebra de contração.
6. **Adaptador de Demonstração Numérica**: Contém um executor denso de tensores multidimensionais para validar numericamente a árvore ótima contra a contração ingênua linear.

---

## Desempenho e Complexidade

O número total de pares de bipartição avaliados sobre todos os subconjuntos não vazios de tamanho $N$ é:
$$\text{Bipartições}(N) = \frac{3^N - 2^{N+1} + 1}{2}$$

| Tensores ($N$) | Subconjuntos ($2^N - 1$) | Bipartições Avaliadas | Consumo de Memória | Tempo de Resolução (GCC 14 / x86_64) |
|:---:|:---:|:---:|:---:|:---:|
| 4 | 15 | 25 | $< 1\,\text{KB}$ | $< 1\,\mu\text{s}$ |
| 6 | 63 | 301 | $3\,\text{KB}$ | $< 8\,\mu\text{s}$ |
| 8 | 255 | 3.025 | $12\,\text{KB}$ | $< 85\,\mu\text{s}$ |
| 10 | 1.023 | 28.501 | $48\,\text{KB}$ | $< 750\,\mu\text{s}$ |
| 12 | 4.095 | 261.625 | $192\,\text{KB}$ | $\approx 4,5\,\text{ms}$ |

---

## Início Rápido e Uso da CLI

### Compilação a partir do Código-Fonte
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
```

### Executar Testes Unitários
```bash
ctest --test-dir build --output-on-failure
```

### Executar Demonstração com Assertions
```bash
./build/tensor-order-cli demo --assert
```

### Emitir Diagnósticos em Formato JSON
```bash
./build/tensor-order-cli demo --json
```

---

## Exemplo de Uso da API C++

```cpp
#include "tensor_order/planner.hpp"
#include <iostream>

int main() {
    using namespace tensor_order;

    // Define a rede: cadeia de 3 matrizes (10x30) x (30x5) x (5x60)
    Network net;
    net.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}}
    };
    net.dimensions = {
        {"i", 10}, {"j", 30}, {"k", 5}, {"l", 60}
    };

    // Valida e compila as máscaras de bits
    std::string err;
    if (!net.validate(err)) {
        std::cerr << "Erro de validação: " << err << std::endl;
        return 1;
    }
    net.compileBitmasks();

    // Planeja a sequência de contração ótima
    Planner planner;
    PlanResult result = planner.plan(net);

    if (result.status == PlanStatus::OptimalPlan) {
        std::cout << "Multiplicações escalares ótimas: " << result.optimalCost << std::endl;
        std::cout << "Bipartições avaliadas: " << result.evaluatedSplits << std::endl;

        // Audita o recibo
        bool valido = verifyReceipt(net, result.receipt);
        std::cout << "Recibo verificado: " << (valido ? "SIM" : "NÃO") << std::endl;
    }

    return 0;
}
```

---

## Documentação Técnica e Formal

Para demonstrações formais, invariantes e arquitetura interna detalhada:
- [Especificação de Contrato](docs/contract.md): Álgebra de contração, limites dimensionais e garantias numéricas.
- [Arquitetura e Design](docs/design.md): Autômato de DP, diagramas de fluxo, layout de memória e complexidade.

---

## Segurança

Para reportar vulnerabilidades, utilize o Private Vulnerability Reporting do GitHub ou consulte [SECURITY.md](SECURITY.md).

---

## Licença

Este projeto é distribuído sob a [Licença MIT](LICENSE).
Copyright © 2026 Lofrano Tecnologia.
