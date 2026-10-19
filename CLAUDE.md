# Devil_Workshop - Claude Code Integration

Este arquivo contém instruções específicas para usar o Claude Code com o projeto Unreal Engine Devil_Workshop.

## Comandos Úteis

### Integração com Unreal Engine via MCP
Para usar o Claude Code para manipular diretamente o editor Unreal Engine:

```bash
# Para usar habilidades MCP do Unreal Engine
/skill unreal-engine-skills-for-claude-code:unreal-mcp
```

### Otimização de Tokens (RTK)
Este projeto está configurado para usar o Rust Token Killer (RTK) para reduzir o consumo de tokens:

```bash
# Verificar economia de tokens
rtk gain

# Ver histórico de uso com economia
rtk gain --history

# Executar comando sem filtragem (para debug)
rtk proxy <comando>
```

### Desenvolvimento Unreal Engine

#### Compilação
```bash
# Compilar o projeto
dotnet build

# Ou usando UE BuildTool diretamente
UE5.7\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project="Devil_Workshop.uproject" -noP4 -platform=Win64 -clientconfig=Development -serverconfig=Development -cook -allmaps -build -stage -pak -archive -archivedirectory="C:\Builds\Devil_Workshop"
```

#### Execução
```bash
# Executar o jogo
.\Binaries\Win64\Devil_Workshop.exe
```

## Estrutura do Código Fonte

- `Source/Devil_Workshop/Public/` - Headers públicos
- `Source/Devil_Workshop/Private/` - Implementações privadas
- `Content/` - Assets do jogo (meshes, texturas, blueprints, etc.)

## Boas Práticas

1. Sempre use `/skill unreal-engine-skills-for-claude-code:unreal-mcp` quando precisar manipular o editor UE diretamente
2. Use RTK para comandos bash que geram muita saída (compilação, log, etc.)
3. O Claude Code automaticamente encaminha comandos bash através do RTK quando disponível
4. Para criar novas ferramentas MCP, use `/skill unreal-engine-skills-for-claude-code:create-toolset`

## Solução de Problemas

Se o RTK não estiver funcionando corretamente:
```bash
rtk --version
which rtk
```

Se você vir "reachingforthejack/rtk" em vez do esperado, você tem o pacote errado instalado.